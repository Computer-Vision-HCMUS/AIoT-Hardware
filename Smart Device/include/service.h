#ifndef SERVICE_H
#define SERVICE_H

/**
 * @file service.h
 * @brief Service layer — bridges UI screens to EdgeApiClient.
 *
 * Configure the edge API provider at startup:
 *   serviceConfigureEdgeApi(edge_api_ptr);
 *
 * When the provider is null, functions fall back to hardcoded mock data
 * so the device stays usable offline.
 *
 * Search tag: // TODO(ai-integration)
 */

#include <cstdint>
#include <string>
#include <vector>
#include "mock_data.h"

class EdgeApiClient;

// ── Provider configuration ────────────────────────────────────────────────
void serviceConfigureEdgeApi(EdgeApiClient* client);

// ── UC1 — Emotion Detection ───────────────────────────────────────────────
EmotionResult runEmotionDetection();

// ── UC1 — Activity Recommendation ────────────────────────────────────────
ActivityCard getRecommendedActivity(const std::string& emotion);

// ── UC2/UC3 — Content ─────────────────────────────────────────────────────
std::vector<Song>           getRecommendedMusic();
std::vector<PodcastEpisode> getRecommendedPodcast();

// ── UC4 — Companion Chat ──────────────────────────────────────────────────
std::string getCompanionReply(const std::string& userMessage);

// TODO(ai-integration): replace with real I2S microphone HAL
void        startAudioCapture();
std::string stopAudioCapture();

// ── UC5 — Insights / Statistics ───────────────────────────────────────────
EmotionDistribution getStatisticsByPeriod(const std::string& period);

#endif  // SERVICE_H
