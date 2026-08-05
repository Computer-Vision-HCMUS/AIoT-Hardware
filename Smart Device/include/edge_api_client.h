#ifndef EDGE_API_CLIENT_H
#define EDGE_API_CLIENT_H

#include <Arduino.h>
#include "mock_data.h"

class NetworkManager;

/**
 * @file edge_api_client.h
 * @brief HTTP client for the EmotiCare AIoT Cloud API (AIoT-Server).
 *
 * Maps all 5 use-cases to the real server endpoints:
 *
 *   UC1 – Emotion check-in
 *         POST /api/emotion-sessions/sync     → save emotion session
 *         POST /api/recommendations/request   → get activity recommendation
 *
 *   UC2 – Content recommendations (emotion-based)
 *         POST /api/media/recommendations     → music + podcast cards
 *
 *   UC3 – Full catalog
 *         POST /api/media/recommendations  (type=song)    → music list
 *         POST /api/media/recommendations  (type=podcast) → podcast list
 *
 *   UC4 – Companion chat
 *         POST /api/conversations/respond
 *
 *   UC5 – Emotion statistics
 *         GET  /api/statistics/day|week|month
 *
 * Authentication:
 *   Every request carries the header  X-Device-Token: <token>
 *   where <token> was obtained during device pairing and stored in NVS.
 *
 * Offline / error behaviour:
 *   All methods return false on any HTTP or JSON error so callers in
 *   service.cpp can fall back to hardcoded mock data.
 */
class EdgeApiClient {
public:
    explicit EdgeApiClient(NetworkManager& network);

    // ── Connectivity ──────────────────────────────────────────────────────

    /** GET /  — returns true if the server root is reachable (no auth needed). */
    bool healthCheck();

    /**
     * POST /api/devices/heartbeat
     * Simplest authenticated ping — confirms Wi-Fi + token + server all work.
     * Logs server_time and status to Serial when EDGE_API_DEBUG=1.
     * Returns true on HTTP 200.
     */
    bool heartbeat();

    /** True only when Wi-Fi is connected and this device has a token. */
    bool canSync() const;

    // ── UC1 — Emotion check-in ────────────────────────────────────────────

    /**
     * Syncs a single emotion session to the server (idempotent).
     *
     * POST /api/emotion-sessions/sync
     *
     * @param emotion      Emotion label (lowercase, e.g. "happy")
     * @param confidence   0.0–1.0
     * @param sessionId    OUT — server-side UUID assigned to this session
     * @return true on success
     */
    bool syncEmotionSession(const String& emotion, float confidence,
                            String& sessionId);

    /**
     * Requests activity recommendation cards for an emotion session.
     *
     * POST /api/recommendations/request  { "session_id": <uuid> }
     *
     * Extracts all "activity" cards and fills @p activities.
     *
     * @param sessionId  UUID returned by syncEmotionSession()
     * @param activities OUT — activity cards (title + description)
     */
    bool getActivityRecommendations(const String& sessionId,
                                    std::vector<ActivityCard>& activities);

    // ── UC2 — Content recommendations ────────────────────────────────────

    /**
     * Returns emotion-matched music and podcast cards.
     *
     * POST /api/media/recommendations
     *   { "emotion_label": <emotion>, "media_type": "both" }
     *
     * @param emotion   Emotion label (lowercase)
     * @param songs     OUT — music cards
     * @param episodes  OUT — podcast cards
     */
    bool getContentRecommendations(const String& emotion,
                                   std::vector<Song>& songs,
                                   std::vector<PodcastEpisode>& episodes,
                                   const char* mediaType = "both");

    // ── UC3 — Full catalog ────────────────────────────────────────────────

    /**
     * POST /api/media/recommendations  { "media_type": "song" }
     */
    bool getMusicCatalog(std::vector<Song>& songs);

    /**
     * POST /api/media/recommendations  { "media_type": "podcast" }
     */
    bool getPodcastCatalog(std::vector<PodcastEpisode>& episodes);

    // ── UC4 — Companion chat ──────────────────────────────────────────────

    /**
     * Sends a user message and returns the AI reply.
     *
     * POST /api/conversations/respond
     *   { "session_id": <uuid>, "user_message": <text> }
     *
     * @param sessionId  Most-recent emotion session UUID (may be empty string
     *                   — server will reject with 404; caller falls back to mock)
     * @param message    User text
     * @param reply      OUT — AI response text
     */
    bool getCompanionReply(const String& sessionId, const String& message,
                           String& reply);

    /** Upload raw 16 kHz signed PCM and receive transcript, reply and audio URL. */
    bool submitCompanionPcm(const String& sessionId, const char* pcmPath,
                            String& transcript, String& reply, String& audioUrl);

    /** Credentials for AudioManager's authenticated PCM GET request. */
    String serverBaseUrl() const;
    String deviceToken() const;

    // ── UC5 — Statistics ──────────────────────────────────────────────────

    /**
     * GET /api/statistics/day|week|month
     *
     * @param period  "Day", "Week", or "Month" (case-insensitive)
     * @param dist    OUT — emotion distribution percentages
     */
    bool getStatistics(const String& period, EmotionDistribution& dist);

    /** Request the cloud AI's plain-language assessment for a statistics period. */
    bool getStatisticsExplanation(const String& period, std::string& explanation);

private:
    // ── HTTP helpers ──────────────────────────────────────────────────────

    /** POST JSON to path; adds auth header. Returns true on 2xx. */
    bool postJson(const char* path, const String& payload, String& response);

    /** GET path; adds auth header. Returns true on 2xx. */
    bool getJson(const char* path, String& response);

    /** True when connected AND paired (token available). */
    bool isReady() const;

    // ── Helper — parse media cards from API response ──────────────────────
    void parseMediaCards(const String& response,
                         std::vector<Song>& songs,
                         std::vector<PodcastEpisode>& episodes);
    void parseMediaLibrary(const String& response,
                           std::vector<Song>& songs,
                           std::vector<PodcastEpisode>& episodes);

    NetworkManager& network_;

    // Most-recent session UUID — cached so companion chat can reuse it
    // without requiring callers to thread it through every function.
    String last_session_id_;
};

#endif  // EDGE_API_CLIENT_H
