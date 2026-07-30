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
#include "pins_config.h"
#include "ser/ser_esp32.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#include <SPIFFS.h>
#include <algorithm>
#include <cmath>
#include <memory>
#include <new>

namespace {
EdgeApiClient* g_edge_api = nullptr;
AudioManager* g_audio_manager = nullptr;
volatile bool g_companion_voice_busy = false;
volatile bool g_companion_voice_result_ready = false;
volatile bool g_companion_voice_success = false;
volatile bool g_companion_audio_started = false;
std::string g_companion_transcript;
std::string g_companion_reply;
String         g_last_session_id;
String         g_last_media_emotion = "neutral";
std::vector<Song> g_music_cache;
std::vector<PodcastEpisode> g_podcast_cache;
unsigned long g_music_cache_at_ms = 0;
unsigned long g_podcast_cache_at_ms = 0;
constexpr unsigned long kMediaCacheTtlMs = 60000;
constexpr size_t kCheckInInferenceBytes = 2 * AUDIO_SAMPLE_RATE * sizeof(int16_t);
constexpr size_t kCheckInMinBytes = AUDIO_SAMPLE_RATE * sizeof(int16_t);
constexpr float kCheckInMinRms = 0.008f;
constexpr float kCheckInConfidenceThreshold = 0.60f;
constexpr const char* kConfirmedEmotionPath = "/confirmed_emotion.json";

// The extractor workspace is about 20 KiB, so it must not live on a task stack.
aiot::ser::esp32::ExtractorWorkspace g_ser_workspace;


void sendCompanionVoiceRequest() {
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
    g_last_media_emotion = result.label.c_str();
    g_last_media_emotion.toLowerCase();

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
            std::vector<Song> aiSongs;
            std::vector<PodcastEpisode> ignoredEpisodes;
            if (g_edge_api->getContentRecommendations(
                    g_last_media_emotion, aiSongs, ignoredEpisodes)) {
                for (Song& song : songs) {
                    song.isAiRecommended = std::any_of(
                        aiSongs.begin(), aiSongs.end(), [&song](const Song& candidate) {
                            return candidate.mediaId == song.mediaId;
                        });
                }
            }
            std::stable_sort(songs.begin(), songs.end(), [](const Song& left, const Song& right) {
                return left.isAiRecommended && !right.isAiRecommended;
            });
            g_music_cache = songs;
            g_music_cache_at_ms = millis();
            return g_music_cache;
        }
    }

    g_music_cache = {
        { "", "Calm Waves",        "Ambient Studio",   "3:45", true,  "" },
        { "", "Morning Dew",       "Nature Sounds",    "4:12", false, "" },
        { "", "Focused Mind",      "Lo-Fi Beats",      "5:01", true,  "" },
        { "", "Gentle Rain",       "Relax Collective", "3:33", false, "" },
        { "", "Soft Piano Dreams", "Luna Keys",        "4:55", false, "" },
        { "", "Tranquil River",    "Zen Music",        "6:02", false, "" },
        { "", "Light Breeze",      "Air Ensemble",     "3:28", false, "" },
        { "", "Soothing Strings",  "Calm Orchestra",   "5:17", false, "" },
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
            std::vector<Song> ignoredSongs;
            std::vector<PodcastEpisode> aiEpisodes;
            if (g_edge_api->getContentRecommendations(
                    g_last_media_emotion, ignoredSongs, aiEpisodes)) {
                for (PodcastEpisode& episode : episodes) {
                    episode.isAiRecommended = std::any_of(
                        aiEpisodes.begin(), aiEpisodes.end(), [&episode](const PodcastEpisode& candidate) {
                            return candidate.mediaId == episode.mediaId;
                        });
                }
            }
            std::stable_sort(episodes.begin(), episodes.end(),
                             [](const PodcastEpisode& left, const PodcastEpisode& right) {
                return left.isAiRecommended && !right.isAiRecommended;
            });
            g_podcast_cache = episodes;
            g_podcast_cache_at_ms = millis();
            return g_podcast_cache;
        }
    }

    g_podcast_cache = {
        { "", "Mindfulness for Beginners", "Calm Daily",        "12:30", true,  "" },
        { "", "Managing Anxiety",          "Mind & Body Talks", "18:45", false, "" },
        { "", "Gratitude Journaling",      "Positive Space",    "10:15", true,  "" },
        { "", "Deep Sleep Techniques",     "Rest Easy Podcast", "22:00", false, "" },
        { "", "Finding Inner Peace",       "Serenity Now",      "15:30", false, "" },
        { "", "Overcoming Daily Stress",   "Wellness Weekly",   "19:20", false, "" },
    };
    g_podcast_cache_at_ms = millis();
    return g_podcast_cache;
}

bool finishCheckInCapture(EmotionResult& result, bool& isUncertain) {
    isUncertain = false;
    if (g_audio_manager == nullptr) return false;

    const size_t recordedBytes = g_audio_manager->recordedBytes();
    if (recordedBytes < kCheckInMinBytes || (recordedBytes % sizeof(int16_t)) != 0) {
        Serial.printf("[CheckIn] Invalid recording size: %u bytes\n", (unsigned)recordedBytes);
        return false;
    }
    // Keep the complete ten-second clip in SPIFFS, but infer on its most
    // recent two seconds.  Loading all 10 seconds (320 KiB) would exhaust
    // the ESP32's RAM before SER can allocate its extractor workspace.
    const size_t byteCount = std::min(recordedBytes, kCheckInInferenceBytes);
    if (!SPIFFS.begin(false)) {
        Serial.println("[CheckIn] SPIFFS unavailable");
        return false;
    }

    File input = SPIFFS.open(g_audio_manager->recordingPath(), FILE_READ);
    if (!input) return false;
    if (recordedBytes > byteCount && !input.seek(recordedBytes - byteCount, SeekSet)) {
        input.close();
        Serial.println("[CheckIn] Cannot seek PCM inference window");
        return false;
    }
    std::unique_ptr<int16_t[]> pcm(new (std::nothrow) int16_t[byteCount / sizeof(int16_t)]);
    if (!pcm || input.readBytes(reinterpret_cast<char*>(pcm.get()), byteCount) != byteCount) {
        input.close();
        Serial.println("[CheckIn] Cannot load PCM capture");
        return false;
    }
    input.close();

    const size_t sampleCount = byteCount / sizeof(int16_t);
    double sumSquares = 0.0;
    for (size_t i = 0; i < sampleCount; ++i) {
        const float sample = pcm[i] / 32768.0f;
        sumSquares += sample * sample;
    }
    const float rms = sqrtf(static_cast<float>(sumSquares / sampleCount));
    if (rms < kCheckInMinRms) {
        Serial.printf("[CheckIn] Clip too quiet: RMS %.4f\n", rms);
        return false;
    }

    aiot::ser::esp32::Prediction prediction{};
    if (!aiot::ser::esp32::classify_pcm(pcm.get(), sampleCount, AUDIO_SAMPLE_RATE,
                                        g_ser_workspace, prediction)) {
        Serial.println("[CheckIn] SER inference failed");
        return false;
    }

    result.label = prediction.label;
    result.confidence = static_cast<uint8_t>(roundf(prediction.confidence * 100.0f));
    isUncertain = prediction.confidence < kCheckInConfidenceThreshold;
    Serial.printf("[CheckIn] %s (%.2f)%s\n", prediction.label, prediction.confidence,
                  isUncertain ? " uncertain" : "");

    return true;
}

bool confirmCheckInEmotion(const std::string& label, uint8_t confidence, bool& synced) {
    synced = false;
    if (!SPIFFS.begin(true)) {
        Serial.println("[CheckIn] Cannot mount SPIFFS for emotion state");
        return false;
    }

    JsonDocument document;
    document["label"] = label;
    document["confidence"] = confidence;
    File output = SPIFFS.open(kConfirmedEmotionPath, FILE_WRITE);
    if (!output || serializeJson(document, output) == 0) {
        if (output) output.close();
        Serial.println("[CheckIn] Cannot save confirmed emotion");
        return false;
    }
    output.close();

    // Do not issue an HTTP request unless Wi-Fi and pairing are both ready.
    if (g_edge_api != nullptr && g_edge_api->canSync()) {
        String sessionId;
        synced = g_edge_api->syncEmotionSession(label.c_str(), confidence / 100.0f, sessionId);
        if (synced) g_last_session_id = sessionId;
    }
    Serial.printf("[CheckIn] Confirmed %s (%u%%), synced=%d\n", label.c_str(), confidence, synced);
    return true;
}

bool loadConfirmedEmotion(std::string& label, uint8_t& confidence) {
    if (!SPIFFS.begin(false)) return false;
    File input = SPIFFS.open(kConfirmedEmotionPath, FILE_READ);
    if (!input) return false;
    JsonDocument document;
    const DeserializationError error = deserializeJson(document, input);
    input.close();
    if (error || !document["label"].is<const char*>()) return false;
    label = document["label"].as<const char*>();
    confidence = document["confidence"] | 0;
    return !label.empty();
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
    // Deliberately synchronous: after SEND the app waits for the complete
    // STT/LLM/TTS result instead of running a separate network task.
    sendCompanionVoiceRequest();
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
        dist.angryPct = 4; dist.calmPct = 22; dist.disgustPct = 2; dist.fearfulPct = 5;
        dist.happyPct = 42; dist.neutralPct = 16; dist.sadPct = 6; dist.surprisedPct = 3;
    } else if (period == "Week") {
        dist.angryPct = 7; dist.calmPct = 18; dist.disgustPct = 3; dist.fearfulPct = 8;
        dist.happyPct = 34; dist.neutralPct = 18; dist.sadPct = 9; dist.surprisedPct = 3;
    } else {
        // Month
        dist.angryPct = 5; dist.calmPct = 20; dist.disgustPct = 3; dist.fearfulPct = 7;
        dist.happyPct = 38; dist.neutralPct = 17; dist.sadPct = 7; dist.surprisedPct = 3;
    }
    return dist;
}

bool getStatisticsAiExplanation(const std::string& period, std::string& explanation) {
    explanation.clear();
    if (!g_edge_api) return false;
    return g_edge_api->getStatisticsExplanation(String(period.c_str()), explanation);
}
