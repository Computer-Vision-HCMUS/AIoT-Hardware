/**
 * @file service.cpp
 * @brief Service layer — bridges UI screens to the EmotiCare AIoT Cloud API.
 *
 * Pattern for every function:
 *   1. If EdgeApiClient is configured, attempt the real API call.
 *   2. On failure (offline / unpaired / HTTP error) fall back to
 *      hardcoded mock data so the device stays usable without a server.
 *
 * UC1 flow (two-step):
 *   a. syncEmotionSession()       → POST /api/emotion-sessions/sync
 *                                   returns server-assigned session UUID
 *   b. getActivityRecommendation() → POST /api/recommendations/request
 *                                   with that UUID
 *
 * UC4 (companion chat) uses the session UUID from the most recent check-in
 * cached inside EdgeApiClient::last_session_id_.
 *
 * Search tag: // TODO(ai-integration)
 */

#include "service.h"
#include "edge_api_client.h"
#include "hal/audio_manager.h"
#include <Arduino.h>

namespace {
EdgeApiClient* g_edge_api = nullptr;
AudioManager* g_audio_manager = nullptr;
TaskHandle_t g_companion_voice_task = nullptr;
volatile bool g_companion_voice_busy = false;
volatile bool g_companion_voice_result_ready = false;
volatile bool g_companion_voice_success = false;
volatile bool g_companion_audio_started = false;
std::string g_companion_transcript;
std::string g_companion_reply;
String         g_last_session_id;
std::vector<Song> g_music_cache;
std::vector<PodcastEpisode> g_podcast_cache;
unsigned long g_music_cache_at_ms = 0;
unsigned long g_podcast_cache_at_ms = 0;
constexpr unsigned long kMediaCacheTtlMs = 60000;

void companionVoiceTask(void*) {
    String remoteTranscript;
    String remoteReply;
    String audioUrl;
    bool ok = false;
    bool audioStarted = false;
    if (g_audio_manager != nullptr && g_edge_api != nullptr &&
        g_audio_manager->recordedBytes() >= 3200) {
        if (g_last_session_id.isEmpty()) {
            Serial.println("[Companion] No check-in session; creating neutral session");
            String sessionId;
            if (g_edge_api->syncEmotionSession("neutral", 0.0f, sessionId)) {
                g_last_session_id = sessionId;
            } else {
                Serial.println("[Companion] Could not create neutral session");
            }
        }
        Serial.printf("[Companion] Sending recording (%u bytes)\n",
                      (unsigned)g_audio_manager->recordedBytes());
        ok = g_edge_api->submitCompanionPcm(g_last_session_id,
                                             g_audio_manager->recordingPath(),
                                             remoteTranscript, remoteReply, audioUrl);
        if (ok && !audioUrl.isEmpty()) {
            audioStarted = g_audio_manager->startStream(audioUrl.c_str(), g_edge_api->deviceToken().c_str());
            if (!audioStarted)
                Serial.println("[Companion] Reply text received but audio playback could not start");
        } else if (ok) {
            Serial.println("[Companion] Reply text received without TTS audio");
        }
    }
    g_companion_transcript = remoteTranscript.c_str();
    g_companion_reply = remoteReply.c_str();
    g_companion_voice_success = ok;
    g_companion_audio_started = audioStarted;
    g_companion_voice_result_ready = true;
    g_companion_voice_busy = false;
    g_companion_voice_task = nullptr;
    vTaskDelete(nullptr);
}
}

void serviceConfigureEdgeApi(EdgeApiClient* client) {
    g_edge_api = client;
}

void serviceConfigureAudioManager(AudioManager* audio) {
    g_audio_manager = audio;
}

// ─────────────────────────────────────────────────────────────────────────────
// UC1 — runEmotionDetection()
//
// Tries: EdgeApiClient::syncEmotionSession()
//        (syncs emotion to server, returns session UUID for later calls)
// Falls back to: hardcoded "Happy / 87%"
//
// TODO(ai-integration): replace stub confidence value with real I2S-derived
//   RMS energy + ZCR once the audio HAL exists.  Pass real confidence to sync.
// ─────────────────────────────────────────────────────────────────────────────
EmotionResult runEmotionDetection() {
    delay(20);  // simulate brief processing (UI only — remove when real SER exists)

    // Stub features — replace with real I2S-derived values
    constexpr float kConfidence = 0.87f;

    EmotionResult result;
    result.label      = "Happy";
    result.confidence = static_cast<uint8_t>(kConfidence * 100.0f);

    if (g_edge_api) {
        String sessionId;
        if (g_edge_api->syncEmotionSession(
                String(result.label.c_str()), kConfidence, sessionId)) {
            g_last_session_id = sessionId;
            // Note: emotion label stays as the stub value — a real SER model
            // would set it before calling sync.
        }
    }

    return result;
}

// ─────────────────────────────────────────────────────────────────────────────
// UC1 — getRecommendedActivity()
//
// Tries: EdgeApiClient::getActivityRecommendation() using cached session UUID
// Falls back to: emotion-sensitive local responses
//
// TODO(ai-integration): pass real confidence value through from detection result
// ─────────────────────────────────────────────────────────────────────────────
ActivityCard getRecommendedActivity(const std::string& emotion) {
    const std::vector<ActivityCard> activities = getRecommendedActivities(emotion);
    return activities.empty() ? ActivityCard{} : activities.front();
}

std::vector<ActivityCard> getRecommendedActivities(const std::string& emotion) {
    delay(15);

    if (g_edge_api && !g_last_session_id.isEmpty()) {
        std::vector<ActivityCard> activities;
        if (g_edge_api->getActivityRecommendations(g_last_session_id, activities)) {
            return activities;
        }
    }

    // MOCK FALLBACK — a list keeps Support usable when the server is unavailable.
    std::vector<ActivityCard> activities;
    ActivityCard card;
    if (emotion == "Anxious" || emotion == "stressed") {
        card.title       = "Box Breathing";
        card.description = "Breathe in 4 s, hold 4 s, breathe out 4 s, hold 4 s. Repeat 4 times.";
    } else if (emotion == "Sad" || emotion == "sad") {
        card.title       = "Small Reset Walk";
        card.description = "Take a 5-minute walk and name three things you can see.";
    } else if (emotion == "Happy" || emotion == "happy") {
        card.title       = "Capture the Moment";
        card.description = "Write one short note about what made today feel lighter.";
    } else {
        card.title       = "Breathing & Light Stretch";
        card.description = "Take 5 deep breaths, then do a 3-minute gentle stretch.";
    }
    activities.push_back(card);
    return activities;
}

// ─────────────────────────────────────────────────────────────────────────────
// UC2/UC3 — getRecommendedMusic()
//
// Server supplies the recommendation catalog. Playback itself remains local,
// so the ESP never downloads or streams an audio URL.
//
// TODO(ai-integration): use getContentRecommendations(currentEmotion) for the
//   emotion-personalised shorter list shown on the Discover screen.
// ─────────────────────────────────────────────────────────────────────────────
std::vector<Song> getRecommendedMusic() {
    if (!g_music_cache.empty() && millis() - g_music_cache_at_ms < kMediaCacheTtlMs) {
        return g_music_cache;
    }

    if (g_edge_api) {
        std::vector<Song> songs;
        if (g_edge_api->getMusicCatalog(songs) && !songs.empty()) {
            g_music_cache = songs;
            g_music_cache_at_ms = millis();
            return g_music_cache;
        }
    }

    g_music_cache = {
        { "Calm Waves",        "Ambient Studio",   "3:45", true,  "" },
        { "Morning Dew",       "Nature Sounds",    "4:12", false, "" },
        { "Focused Mind",      "Lo-Fi Beats",      "5:01", true,  "" },
        { "Gentle Rain",       "Relax Collective", "3:33", false, "" },
        { "Soft Piano Dreams", "Luna Keys",        "4:55", false, "" },
        { "Tranquil River",    "Zen Music",        "6:02", false, "" },
        { "Light Breeze",      "Air Ensemble",     "3:28", false, "" },
        { "Soothing Strings",  "Calm Orchestra",   "5:17", false, "" },
    };
    g_music_cache_at_ms = millis();
    return g_music_cache;
}

// ─────────────────────────────────────────────────────────────────────────────
// UC2/UC3 — getRecommendedPodcast()
//
// Server supplies the recommendation catalog. Playback itself remains local,
// so the ESP never downloads or streams an audio URL.
// ─────────────────────────────────────────────────────────────────────────────
std::vector<PodcastEpisode> getRecommendedPodcast() {
    if (!g_podcast_cache.empty() && millis() - g_podcast_cache_at_ms < kMediaCacheTtlMs) {
        return g_podcast_cache;
    }

    if (g_edge_api) {
        std::vector<PodcastEpisode> episodes;
        if (g_edge_api->getPodcastCatalog(episodes) && !episodes.empty()) {
            g_podcast_cache = episodes;
            g_podcast_cache_at_ms = millis();
            return g_podcast_cache;
        }
    }

    g_podcast_cache = {
        { "Mindfulness for Beginners", "Calm Daily",        "12:30", true,  "" },
        { "Managing Anxiety",          "Mind & Body Talks", "18:45", false, "" },
        { "Gratitude Journaling",      "Positive Space",    "10:15", true,  "" },
        { "Deep Sleep Techniques",     "Rest Easy Podcast", "22:00", false, "" },
        { "Finding Inner Peace",       "Serenity Now",      "15:30", false, "" },
        { "Overcoming Daily Stress",   "Wellness Weekly",   "19:20", false, "" },
    };
    g_podcast_cache_at_ms = millis();
    return g_podcast_cache;
}

// ─────────────────────────────────────────────────────────────────────────────
// UC4 — getCompanionReply()
//
// Tries: EdgeApiClient::getCompanionReply()
//   Uses the cached session UUID from the most recent check-in.
//   If no session exists yet the API call fails and mock is returned.
// Falls back to: rotating 4-reply mock
//
// TODO(ai-integration): replace with real LLM / conversational AI backend
// ─────────────────────────────────────────────────────────────────────────────
std::string getCompanionReply(const std::string& userMessage) {
    if (g_edge_api) {
        String remoteReply;
        if (g_edge_api->getCompanionReply(
                g_last_session_id,
                String(userMessage.c_str()),
                remoteReply)) {
            return std::string(remoteReply.c_str());
        }
    }

    delay(20);

    // MOCK FALLBACK — rotating replies
    static uint8_t idx = 0;
    static const char* kReplies[] = {
        "I'm here to help! How are you feeling?",
        "Thanks for sharing that with me.",
        "That sounds meaningful. Tell me more.",
        "I hear you. You're doing great.",
    };
    constexpr uint8_t kCount = 4;
    const char* reply = kReplies[idx % kCount];
    ++idx;
    return std::string(reply);
}

// ─────────────────────────────────────────────────────────────────────────────
// UC4 — Audio capture stubs
//
// TODO(ai-integration): replace with real I2S microphone HAL:
//   1. startAudioCapture() → i2s_start() + allocate DMA buffer
//   2. stopAudioCapture()  → i2s_stop() + call uploadAudio() with buffer
// ─────────────────────────────────────────────────────────────────────────────
// ─────────────────────────────────────────────────────────────────────────────
// UC4 — Audio capture stubs
//
// TODO(ai-integration): replace with real I2S microphone HAL + STT pipeline
// ─────────────────────────────────────────────────────────────────────────────
bool startAudioCapture(bool resume) {
    return g_audio_manager != nullptr && g_audio_manager->startRecording(resume);
}

void pauseAudioCapture() {
    if (g_audio_manager != nullptr) g_audio_manager->pauseRecording();
}

bool beginCompanionVoiceRequest() {
    if (g_companion_voice_busy || g_audio_manager == nullptr || g_edge_api == nullptr ||
        g_audio_manager->recordedBytes() < 3200) {
        Serial.printf("[Companion] Cannot queue request: busy=%d audio=%d api=%d bytes=%u\n",
                      g_companion_voice_busy, g_audio_manager != nullptr, g_edge_api != nullptr,
                      g_audio_manager == nullptr ? 0U : (unsigned)g_audio_manager->recordedBytes());
        return false;
    }
    g_companion_voice_result_ready = false;
    g_companion_voice_busy = true;
    if (xTaskCreate(companionVoiceTask, "companion_net", 10240, nullptr, 4,
                    &g_companion_voice_task) != pdPASS) {
        g_companion_voice_busy = false;
        g_companion_voice_task = nullptr;
        return false;
    }
    Serial.println("[Companion] Voice request queued");
    return true;
}

bool takeCompanionVoiceResult(std::string& transcript, std::string& reply,
                              bool& success, bool& audioStarted) {
    if (!g_companion_voice_result_ready) return false;
    transcript = g_companion_transcript;
    reply = g_companion_reply;
    success = g_companion_voice_success;
    audioStarted = g_companion_audio_started;
    g_companion_voice_result_ready = false;
    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// UC5 — getStatisticsByPeriod()
//
// Tries: EdgeApiClient::getStatistics()
//   GET /api/statistics/day|week|month
// Falls back to: hardcoded distribution tables
//
// TODO(ai-integration): replace fallback with real aggregation from NVS once
//   the device stores emotion sessions locally.
// ─────────────────────────────────────────────────────────────────────────────
EmotionDistribution getStatisticsByPeriod(const std::string& period) {
    delay(10);

    if (g_edge_api) {
        EmotionDistribution dist;
        if (g_edge_api->getStatistics(String(period.c_str()), dist)) {
            return dist;
        }
    }

    // MOCK FALLBACK
    EmotionDistribution dist;
    dist.period = period;

    if (period == "Day") {
        dist.happyPct   = 60; dist.calmPct    = 25; dist.focusedPct = 10;
        dist.sadPct     =  3; dist.anxiousPct =  2;
    } else if (period == "Week") {
        dist.happyPct   = 45; dist.calmPct    = 30; dist.focusedPct = 15;
        dist.sadPct     =  7; dist.anxiousPct =  3;
    } else {
        // Month
        dist.happyPct   = 50; dist.calmPct    = 28; dist.focusedPct = 12;
        dist.sadPct     =  6; dist.anxiousPct =  4;
    }
    return dist;
}
