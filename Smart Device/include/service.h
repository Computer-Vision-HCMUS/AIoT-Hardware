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
class AudioManager;

// ── Provider configuration ────────────────────────────────────────────────
void serviceConfigureEdgeApi(EdgeApiClient* client);
void serviceConfigureAudioManager(AudioManager* audio);

// ── UC1 — Emotion Detection ───────────────────────────────────────────────
EmotionResult runEmotionDetection();

/** Run local SER on the recorded Check-In PCM clip. */
bool finishCheckInCapture(EmotionResult& result, bool& isUncertain);
bool confirmCheckInEmotion(const std::string& label, uint8_t confidence, bool& synced);
bool loadConfirmedEmotion(std::string& label, uint8_t& confidence);


// ── UC1 — Activity Recommendation ────────────────────────────────────────
ActivityCard getRecommendedActivity(const std::string& emotion);
std::vector<ActivityCard> getRecommendedActivities(const std::string& emotion);

// ── UC2/UC3 — Content ─────────────────────────────────────────────────────
std::vector<Song>           getRecommendedMusic();
std::vector<PodcastEpisode> getRecommendedPodcast();

// ── UC4 — Companion Chat ──────────────────────────────────────────────────
std::string getCompanionReply(const std::string& userMessage);

bool startAudioCapture(bool resume = false);
void pauseAudioCapture();
bool beginCompanionVoiceRequest();
bool takeCompanionVoiceResult(std::string& transcript, std::string& reply,
                              bool& success, bool& audioStarted);

// ── UC5 — Insights / Statistics ───────────────────────────────────────────
EmotionDistribution getStatisticsByPeriod(const std::string& period);
bool getStatisticsAiExplanation(const std::string& period, std::string& explanation);

#endif  // SERVICE_H
