#ifndef MOCK_DATA_H
#define MOCK_DATA_H

#include <cstdint>
#include <string>
#include <vector>

// ---------------------------------------------------------------------------
// EmotionResult — returned by runEmotionDetection()
// ---------------------------------------------------------------------------
struct EmotionResult {
    std::string label;      // e.g. "Happy", "Calm", "Focused"
    uint8_t     confidence; // 0–100 (percentage)
};

// ---------------------------------------------------------------------------
// ActivityCard — returned by getRecommendedActivity()
// ---------------------------------------------------------------------------
struct ActivityCard {
    std::string title;
    std::string description;
    std::string actionId;  // Server identifier, e.g. "activity:breathing"
};

// ---------------------------------------------------------------------------
// Song — element of getRecommendedMusic() result
// ---------------------------------------------------------------------------
struct Song {
    std::string title;
    std::string artist;
    std::string duration;       // e.g. "3:45"
    bool        isAiRecommended;
    std::string sourceUrl;      // Direct HTTP(S) MP3 stream supplied by the server
};

// ---------------------------------------------------------------------------
// PodcastEpisode — element of getRecommendedPodcast() result
// ---------------------------------------------------------------------------
struct PodcastEpisode {
    std::string title;
    std::string creator;
    std::string duration;       // e.g. "12:30"
    bool        isAiRecommended;
    std::string sourceUrl;      // Direct HTTP(S) MP3 stream supplied by the server
};

// ---------------------------------------------------------------------------
// EmotionDistribution — returned by getStatisticsByPeriod()
// ---------------------------------------------------------------------------
struct EmotionDistribution {
    std::string period;         // "Day", "Week", or "Month"
    uint8_t     happyPct;       // 0–100
    uint8_t     calmPct;
    uint8_t     focusedPct;
    uint8_t     sadPct;
    uint8_t     anxiousPct;
};

// ---------------------------------------------------------------------------
// ChatMessage — element of the conversation history
// ---------------------------------------------------------------------------
struct ChatMessage {
    std::string sender;  // "user" or "ai"
    std::string text;
    uint32_t    timestamp; // optional ordering key
};

#endif  // MOCK_DATA_H
