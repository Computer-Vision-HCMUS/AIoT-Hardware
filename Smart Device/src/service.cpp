/**
 * @file service.cpp
 * @brief Mock service implementations for EmotiCare UI Shell (spec 005)
 *
 * All implementations in this file are HARDCODED MOCK DATA.
 * No Wi-Fi, HTTP, cloud API, real microphone input, or audio encoding is used.
 *
 * Each function is tagged: // TODO(ai-integration)
 * Replace with real implementations when Edge AI / cloud services are ready.
 */

#include "service.h"
#include <Arduino.h>

// ---------------------------------------------------------------------------
// runEmotionDetection()
// TODO(ai-integration): replace with real SER model inference on captured audio
// ---------------------------------------------------------------------------
EmotionResult runEmotionDetection() {
    // Simulated processing delay (UI only — no real audio capture)
    delay(20);

    EmotionResult result;
    result.label      = "Happy";
    result.confidence = 87;
    return result;
}

// ---------------------------------------------------------------------------
// getRecommendedActivity()
// TODO(ai-integration): replace with real recommendation engine / cloud payload
// ---------------------------------------------------------------------------
ActivityCard getRecommendedActivity(const std::string& emotion) {
    (void)emotion;  // suppress unused warning
    delay(15);

    ActivityCard card;
    card.title       = "Breathing & Light Stretch";
    card.description = "Take 5 deep breaths, then do a 3-minute gentle stretch.";
    return card;
}

// ---------------------------------------------------------------------------
// getRecommendedMusic()
// TODO(ai-integration): replace with real music catalog integration
// ---------------------------------------------------------------------------
std::vector<Song> getRecommendedMusic() {
    delay(15);
    return {
        // isAiRecommended = true items must render with distinct visual
        { "Calm Waves",          "Ambient Studio",  "3:45", true  },
        { "Morning Dew",         "Nature Sounds",   "4:12", false },
        { "Focused Mind",        "Lo-Fi Beats",     "5:01", true  },
        { "Gentle Rain",         "Relax Collective","3:33", false },
        { "Soft Piano Dreams",   "Luna Keys",       "4:55", false },
        { "Tranquil River",      "Zen Music",       "6:02", false },
        { "Light Breeze",        "Air Ensemble",    "3:28", false },
        { "Soothing Strings",    "Calm Orchestra",  "5:17", false },
    };
}

// ---------------------------------------------------------------------------
// getRecommendedPodcast()
// TODO(ai-integration): replace with real podcast catalog integration
// ---------------------------------------------------------------------------
std::vector<PodcastEpisode> getRecommendedPodcast() {
    delay(15);
    return {
        // isAiRecommended = true items must render with distinct visual
        { "Mindfulness for Beginners",   "Calm Daily",        "12:30", true  },
        { "Managing Anxiety",            "Mind & Body Talks", "18:45", false },
        { "Gratitude Journaling",        "Positive Space",    "10:15", true  },
        { "Deep Sleep Techniques",       "Rest Easy Podcast", "22:00", false },
        { "Finding Inner Peace",         "Serenity Now",      "15:30", false },
        { "Overcoming Daily Stress",     "Wellness Weekly",   "19:20", false },
    };
}

// ---------------------------------------------------------------------------
// getCompanionReply()
// TODO(ai-integration): replace with real LLM / conversational AI backend
// ---------------------------------------------------------------------------
std::string getCompanionReply(const std::string& userMessage) {
    (void)userMessage;
    delay(20);
    // Rotating mock replies for a slightly more realistic feel
    static uint8_t idx = 0;
    static const char* replies[] = {
        "I'm here to help! How are you feeling?",
        "Thanks for sharing that with me.",
        "That sounds meaningful. Tell me more.",
        "I hear you. You're doing great.",
    };
    constexpr uint8_t kCount = 4;
    const char* reply = replies[idx % kCount];
    idx++;
    return std::string(reply);
}

// ---------------------------------------------------------------------------
// startAudioCapture()
// TODO(ai-integration): replace with real microphone input initialization
// NOTE: This phase is UI simulation ONLY. No actual audio is captured.
// ---------------------------------------------------------------------------
void startAudioCapture() {
    // UI simulation: recording state is managed in AppState.sharedContext.isRecording
    // No microphone, no audio encoding, no transmission.
}

// ---------------------------------------------------------------------------
// stopAudioCapture()
// TODO(ai-integration): replace with real microphone stop + Speech-to-Text
// NOTE: This phase is UI simulation ONLY. Returns a hardcoded mock transcript.
// ---------------------------------------------------------------------------
std::string stopAudioCapture() {
    // Mock transcribed text from a simulated recording session
    return "Hi AI, how are you?";
}

// ---------------------------------------------------------------------------
// getStatisticsByPeriod()
// TODO(ai-integration): replace with real report aggregation from persistent data
// ---------------------------------------------------------------------------
EmotionDistribution getStatisticsByPeriod(const std::string& period) {
    (void)period;
    delay(10);

    EmotionDistribution dist;
    dist.period = period;

    if (period == "Day") {
        dist.happyPct   = 60;
        dist.calmPct    = 25;
        dist.focusedPct = 10;
        dist.sadPct     =  3;
        dist.anxiousPct =  2;
    } else if (period == "Week") {
        dist.happyPct   = 45;
        dist.calmPct    = 30;
        dist.focusedPct = 15;
        dist.sadPct     =  7;
        dist.anxiousPct =  3;
    } else {
        // Month
        dist.happyPct   = 50;
        dist.calmPct    = 28;
        dist.focusedPct = 12;
        dist.sadPct     =  6;
        dist.anxiousPct =  4;
    }
    return dist;
}
