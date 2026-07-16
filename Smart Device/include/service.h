#ifndef SERVICE_H
#define SERVICE_H

/**
 * @file service.h
 * @brief Mock service API for EmotiCare UI Shell (spec 005)
 *
 * All functions declared here are PLACEHOLDER implementations returning
 * hardcoded mock data. They are isolated in this module for easy removal
 * and replacement with real Edge AI / cloud integrations in future phases.
 *
 * Search tag: // TODO(ai-integration)
 */

#include <cstdint>
#include <string>
#include <vector>
#include "mock_data.h"

// ---------------------------------------------------------------------------
// Emotion Detection
// ---------------------------------------------------------------------------

// TODO(ai-integration): replace with real Edge AI Speech Emotion Recognition
EmotionResult runEmotionDetection();

// ---------------------------------------------------------------------------
// Activity Recommendation
// ---------------------------------------------------------------------------

// TODO(ai-integration): replace with real cloud recommendation payload
ActivityCard getRecommendedActivity(const std::string& emotion);

// ---------------------------------------------------------------------------
// Content Recommendations
// ---------------------------------------------------------------------------

// TODO(ai-integration): replace with real music catalog integration
std::vector<Song> getRecommendedMusic();

// TODO(ai-integration): replace with real podcast catalog integration
std::vector<PodcastEpisode> getRecommendedPodcast();

// ---------------------------------------------------------------------------
// Companion Chat
// ---------------------------------------------------------------------------

// TODO(ai-integration): replace with real LLM / chat backend
std::string getCompanionReply(const std::string& userMessage);

// TODO(ai-integration): replace with real microphone input start
void startAudioCapture();

// TODO(ai-integration): replace with real microphone input stop + STT
std::string stopAudioCapture();

// ---------------------------------------------------------------------------
// Insights / Statistics
// ---------------------------------------------------------------------------

// TODO(ai-integration): replace with real report aggregation from persisted data
EmotionDistribution getStatisticsByPeriod(const std::string& period);

#endif  // SERVICE_H
