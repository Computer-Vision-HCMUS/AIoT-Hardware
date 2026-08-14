/**
 * @file edge_api_client.cpp
 * @brief HTTP client implementation — EmotiCare AIoT Cloud API.
 *
 * All methods follow the same pattern:
 *   1. Guard: isReady() — returns false if offline or unpaired.
 *   2. Build JSON request body.
 *   3. Call postJson() / getJson() — adds X-Device-Token header automatically.
 *   4. Deserialize response and fill output parameter(s).
 *   5. Return false on any error so service.cpp falls back to mock data.
 *
 * Enable verbose logs:  add  -DEDGE_API_DEBUG=1  to platformio.ini build_flags.
 *
 * ─── Endpoint mapping ────────────────────────────────────────────────────────
 *  UC1  POST /api/emotion-sessions/sync       syncEmotionSession()
 *       POST /api/recommendations/request     getActivityRecommendation()
 *  UC2  POST /api/media/recommendations       getContentRecommendations()
 *  UC3  POST /api/media/recommendations       getMusicCatalog() / getPodcastCatalog()
 *  UC4  POST /api/conversations/respond       getCompanionReply()
 *  UC5  GET  /api/statistics/{day|week|month} getStatistics()
 * ─────────────────────────────────────────────────────────────────────────────
 */

#include "edge_api_client.h"

#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <SPIFFS.h>
#include <esp_system.h>
#include "network_manager.h"

#ifndef EDGE_API_DEBUG
#define EDGE_API_DEBUG 0
#endif

#if EDGE_API_DEBUG
#define EDGE_LOG(fmt, ...) Serial.printf("[EdgeAPI] " fmt "\n", ##__VA_ARGS__)
#else
#define EDGE_LOG(fmt, ...)
#endif

// Always-on diagnostic macro for critical statistics failures.
// Unlike EDGE_LOG this is never compiled out — it fires regardless of EDGE_API_DEBUG
// so failures are visible on the serial monitor without a special build.
#define STATS_LOG(fmt, ...) Serial.printf("[Stats] " fmt "\n", ##__VA_ARGS__)
// One concise line per HTTP request; sensitive bodies and credentials are omitted.
#define API_LOG(method, path, status) Serial.printf("[API] %s %s -> %d\n", method, path, status)

// ─────────────────────────────────────────────────────────────────────────────
// Constructor
// ─────────────────────────────────────────────────────────────────────────────

EdgeApiClient::EdgeApiClient(NetworkManager& network) : network_(network) {}

// ─────────────────────────────────────────────────────────────────────────────
// Private helpers
// ─────────────────────────────────────────────────────────────────────────────

bool EdgeApiClient::isReady() const {
    if (!network_.isConnected()) {
        STATS_LOG("not connected — skip");
        EDGE_LOG("not connected — skip");
        return false;
    }
    if (!network_.isPaired()) {
        STATS_LOG("not paired — skip");
        EDGE_LOG("not paired — skip");
        return false;
    }
    if (network_.serverBaseUrl().isEmpty()) {
        STATS_LOG("server URL empty — skip");
        EDGE_LOG("server URL empty — skip");
        return false;
    }
    return true;
}

bool EdgeApiClient::postJson(const char* path, const String& payload,
                             String& response) {
    if (!isReady()) return false;

    const String url = network_.serverBaseUrl() + path;
    EDGE_LOG("POST %s  body=%s", url.c_str(), payload.c_str());

    HTTPClient http;
    http.setTimeout(8000);
    if (!http.begin(url)) {
        API_LOG("POST", path, -1);
        EDGE_LOG("http.begin failed for %s", url.c_str());
        return false;
    }

    http.addHeader("X-Device-Token", network_.deviceToken());
    http.addHeader("Content-Type",   "application/json");
    const int status = http.POST(payload);
    API_LOG("POST", path, status);

    if (status >= 200 && status < 300) {
        response = http.getString();
        EDGE_LOG("HTTP %d  resp=%s", status, response.c_str());
    } else {
        EDGE_LOG("HTTP error %d  path=%s", status, path);
    }

    http.end();
    return status >= 200 && status < 300;
}

bool EdgeApiClient::getJson(const char* path, String& response) {
    if (!isReady()) return false;

    const String url = network_.serverBaseUrl() + path;
    EDGE_LOG("GET %s", url.c_str());

    HTTPClient http;
    http.setTimeout(8000);
    if (!http.begin(url)) {
        API_LOG("GET", path, -1);
        EDGE_LOG("http.begin failed for %s", url.c_str());
        return false;
    }

    // GET requests still need the auth header (no body, no Content-Type)
    http.addHeader("X-Device-Token", network_.deviceToken());
    const int status = http.GET();
    API_LOG("GET", path, status);

    if (status >= 200 && status < 300) {
        response = http.getString();
        EDGE_LOG("HTTP %d  resp=%s", status, response.c_str());
    } else {
        STATS_LOG("HTTP error %d  path=%s", status, path);
        EDGE_LOG("HTTP error %d  path=%s", status, path);
    }

    http.end();
    return status >= 200 && status < 300;
}

// ─────────────────────────────────────────────────────────────────────────────
// Connectivity
// ─────────────────────────────────────────────────────────────────────────────

bool EdgeApiClient::healthCheck() {
    // This endpoint is intentionally unauthenticated.  Keep it separate from
    // getJson(), whose isReady() guard requires a paired device, so it can be
    // used to diagnose a server before pairing has completed.
    if (!network_.isConnected() || network_.serverBaseUrl().isEmpty()) {
        return false;
    }

    const String url = network_.serverBaseUrl() + "/";
    String response;

    HTTPClient http;
    http.setTimeout(8000);
    if (!http.begin(url)) {
        API_LOG("GET", "/", -1);
        Serial.printf("[EdgeAPI] Health check could not open %s\n", url.c_str());
        return false;
    }

    const int status = http.GET();
    API_LOG("GET", "/", status);
    if (status >= 200 && status < 300) {
        response = http.getString();
    }
    http.end();

    return status >= 200 && status < 300;
}

// ─────────────────────────────────────────────────────────────────────────────
// Heartbeat — simplest authenticated test
//
// POST /api/devices/heartbeat
// Request:  {} (empty — firmware_version optional)
// Response: { "device_id":"...", "server_time":"...",
//             "status":"online", "config_version":"..." }
// ─────────────────────────────────────────────────────────────────────────────

bool EdgeApiClient::heartbeat() {
    JsonDocument req;
    req["firmware_version"] = "1.0.0";

    String payload;
    serializeJson(req, payload);

    String response;
    if (!postJson("/api/devices/heartbeat", payload, response)) return false;

    JsonDocument json;
    if (deserializeJson(json, response) != DeserializationError::Ok) return false;

    EDGE_LOG("heartbeat OK — status=%s  server_time=%s",
             (const char*)(json["status"]      | "?"),
             (const char*)(json["server_time"] | "?"));
    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// UC1 — Sync emotion session
//
// POST /api/emotion-sessions/sync
// Request:
//   {
//     "sessions": [{
//       "client_session_id": "<unique-id>",
//       "emotion_label":     "happy",
//       "confidence_score":  0.87,
//       "quality_flag":      "clean",
//       "client_created_at": "2025-07-21T12:00:00Z"
//     }]
//   }
// Response:
//   { "received_count": 1, "total_submitted": 1, "received_ids": ["<uuid>"] }
//
// NOTE: The server's received_ids contains client_session_id values (not
//       server-side UUIDs).  To get a server session UUID for use with
//       /api/recommendations/request we must GET /api/emotion-sessions after sync.
//       As a simpler approach we store a UUID in client_session_id ourselves so
//       the same ID can be reused.  The server assigns its own internal UUID but
//       we only need any valid session_id — so we GET the most recent session.
// ─────────────────────────────────────────────────────────────────────────────

bool EdgeApiClient::syncEmotionSession(const String& emotion, float confidence,
                                       String& sessionId) {
    // Keep the idempotency key unique across restarts.  millis() resets on
    // reboot, so it cannot be used by itself or new sessions may be ignored.
    const String clientId = "esp32-" + String(esp_random(), HEX) + "-" + String(millis(), HEX);

    // Normalise emotion label to lowercase (server uses lowercase)
    String label = emotion;
    label.toLowerCase();
    // Map display labels → server emotion labels
    if      (label == "anxious")  label = "stressed";
    else if (label == "focused")  label = "neutral";
    else if (label == "calm")     label = "calm";
    // The embedded RAVDESS classifier has labels outside the cloud's strict
    // EmotionLabel contract.  Normalise them before request validation.
    else if (label == "fearful")  label = "fearful";
    else if (label == "disgust")  label = "disgust";
    else if (label == "surprised") label = "surprised";

    // Determine quality flag based on confidence
    const char* qualityFlag = (confidence >= 0.70f) ? "clean"
                            : (confidence >= 0.50f) ? "noisy"
                            : "low_confidence";

    // Build ISO-8601 timestamp (approximate, no RTC available)
    // Use a fixed epoch-relative string; server stores it but doesn't validate strictly.
    char isoTime[32];
    const unsigned long secs = millis() / 1000UL;
    snprintf(isoTime, sizeof(isoTime),
             "2025-01-01T%02lu:%02lu:%02luZ",
             (secs / 3600) % 24, (secs / 60) % 60, secs % 60);

    JsonDocument req;
    JsonArray sessions = req["sessions"].to<JsonArray>();
    JsonObject s       = sessions.add<JsonObject>();
    s["client_session_id"]  = clientId;
    s["emotion_label"]      = label;
    s["confidence_score"]   = confidence;
    s["quality_flag"]       = qualityFlag;
    s["client_created_at"]  = isoTime;

    String payload;
    serializeJson(req, payload);

    String response;
    if (!postJson("/api/emotion-sessions/sync", payload, response)) return false;

    JsonDocument json;
    if (deserializeJson(json, response) != DeserializationError::Ok) return false;

    // Sync succeeded — now fetch the server-side session UUID
    // GET /api/emotion-sessions?limit=1
    String listResponse;
    if (!getJson("/api/emotion-sessions?limit=1", listResponse)) return false;

    JsonDocument listJson;
    if (deserializeJson(listJson, listResponse) != DeserializationError::Ok) return false;

    JsonArray items = listJson["items"].as<JsonArray>();
    if (items.size() == 0) return false;

    sessionId      = (const char*)(items[0]["id"] | "");
    last_session_id_ = sessionId;
    return !sessionId.isEmpty();
}

// ─────────────────────────────────────────────────────────────────────────────
// UC1 — Activity recommendation
//
// POST /api/recommendations/request
// Request:  { "session_id": "<uuid>" }
// Response: { "recommendation_id": "...", "emotion_label": "...",
//             "cards": [{ "type": "activity", "title": "...", "body": "...", ... }],
//             "status": "success" }
// ─────────────────────────────────────────────────────────────────────────────

bool EdgeApiClient::getActivityRecommendations(
    const String& sessionId, std::vector<ActivityCard>& activities) {
    activities.clear();

    JsonDocument req;
    req["session_id"] = sessionId;

    String payload;
    serializeJson(req, payload);

    String response;
    if (!postJson("/api/recommendations/request", payload, response)) return false;

    JsonDocument json;
    if (deserializeJson(json, response) != DeserializationError::Ok) return false;

    // Keep every activity card so the Support screen can show a selectable list.
    for (JsonObject card : json["cards"].as<JsonArray>()) {
        const String type = card["type"] | "";
        if (type == "activity") {
            ActivityCard activity;
            activity.title       = (const char*)(card["title"] | "");
            activity.description = (const char*)(card["body"]  | "");
            activity.actionId    = (const char*)(card["action_id"] | "");
            if (!activity.title.empty()) {
                activities.push_back(activity);
            }
        }
    }
    return !activities.empty();
}

// ─────────────────────────────────────────────────────────────────────────────
// UC2 / UC3 — Media helper
//
// POST /api/media/recommendations
// Request:  { "emotion_label": "...", "media_type": "both"|"song"|"podcast" }
// Response: { "category": "...", "media_type": "...",
//             "cards": [{ "media_id": "...", "media_type": "song"|"podcast",
//                         "title": "...", "creator": "...",
//                         "duration_sec": 225, "source_url": "...", ... }] }
// ─────────────────────────────────────────────────────────────────────────────

void EdgeApiClient::parseMediaCards(const String& response,
                                    std::vector<Song>& songs,
                                    std::vector<PodcastEpisode>& episodes) {
    JsonDocument json;
    if (deserializeJson(json, response) != DeserializationError::Ok) return;

    for (JsonObject card : json["cards"].as<JsonArray>()) {
        const String type    = card["media_type"]  | "";
        const String mediaId = card["media_id"]    | "";
        const String title   = card["title"]       | "";
        const String category = card["category"]   | "";
        const String source  = card["source_url"]  | "";
        const int    durSec  = card["duration_sec"] | 0;

        // Convert duration_sec to "MM:SS" string
        char durStr[8];
        snprintf(durStr, sizeof(durStr), "%d:%02d", durSec / 60, durSec % 60);

        if (type == "song" && !title.isEmpty()) {
            Song s;
            s.mediaId         = mediaId.c_str();
            s.title           = title.c_str();
            s.artist          = category.c_str();
            s.duration        = durStr;
            s.isAiRecommended = true;
            s.sourceUrl       = source.c_str();
            songs.push_back(s);
        } else if (type == "podcast" && !title.isEmpty()) {
            PodcastEpisode ep;
            ep.mediaId         = mediaId.c_str();
            ep.title           = title.c_str();
            ep.creator         = category.c_str();
            ep.duration        = durStr;
            ep.isAiRecommended = true;
            ep.sourceUrl       = source.c_str();
            episodes.push_back(ep);
        }
    }
}

void EdgeApiClient::parseMediaLibrary(const String& response,
                                      std::vector<Song>& songs,
                                      std::vector<PodcastEpisode>& episodes) {
    JsonDocument json;
    if (deserializeJson(json, response) != DeserializationError::Ok) return;

    for (JsonObject item : json["music"].as<JsonArray>()) {
        const String title = item["title"] | "";
        if (title.isEmpty()) continue;
        const int durationSec = item["duration_sec"] | 0;
        char duration[8];
        snprintf(duration, sizeof(duration), "%d:%02d", durationSec / 60, durationSec % 60);
        songs.push_back({
            (const char*)(item["media_id"] | ""), title.c_str(),
            (const char*)(item["category"] | ""), duration, false,
            (const char*)(item["source_url"] | ""),
        });
    }

    for (JsonObject item : json["podcasts"].as<JsonArray>()) {
        const String title = item["title"] | "";
        if (title.isEmpty()) continue;
        const int durationSec = item["duration_sec"] | 0;
        char duration[8];
        snprintf(duration, sizeof(duration), "%d:%02d", durationSec / 60, durationSec % 60);
        episodes.push_back({
            (const char*)(item["media_id"] | ""), title.c_str(),
            (const char*)(item["category"] | ""), duration, false,
            (const char*)(item["source_url"] | ""),
        });
    }
}

// ── UC2 — Emotion-based music + podcast ──────────────────────────────────────

bool EdgeApiClient::getContentRecommendations(const String& emotion,
                                              std::vector<Song>& songs,
                                              std::vector<PodcastEpisode>& episodes,
                                              const char* mediaType) {
    String label = emotion;
    label.toLowerCase();

    JsonDocument req;
    req["emotion_label"] = label;
    req["media_type"]    = mediaType;

    String payload;
    serializeJson(req, payload);

    String response;
    if (!postJson("/api/media/recommendations", payload, response)) return false;

    songs.clear();
    episodes.clear();
    parseMediaCards(response, songs, episodes);
    return !songs.empty() || !episodes.empty();
}

// ── UC3 — Full music catalog ──────────────────────────────────────────────────

bool EdgeApiClient::getMusicCatalog(std::vector<Song>& songs) {
    String response;
    if (!getJson("/api/media/library", response)) return false;

    songs.clear();
    JsonDocument json;
    if (deserializeJson(json, response) != DeserializationError::Ok) return false;

    for (JsonObject item : json["music"].as<JsonArray>()) {
        const String title = item["title"] | "";
        if (title.isEmpty()) continue;
        const int durationSec = item["duration_sec"] | 0;
        char duration[8];
        snprintf(duration, sizeof(duration), "%d:%02d", durationSec / 60, durationSec % 60);
        songs.push_back({
            (const char*)(item["media_id"] | ""), title.c_str(),
            (const char*)(item["category"] | ""), duration, false,
            (const char*)(item["source_url"] | ""),
        });
    }
    return !songs.empty();
}

// ── UC3 — Full podcast catalog ────────────────────────────────────────────────

bool EdgeApiClient::getPodcastCatalog(std::vector<PodcastEpisode>& episodes) {
    String response;
    if (!getJson("/api/media/library", response)) return false;

    episodes.clear();
    JsonDocument json;
    if (deserializeJson(json, response) != DeserializationError::Ok) return false;

    for (JsonObject item : json["podcasts"].as<JsonArray>()) {
        const String title = item["title"] | "";
        if (title.isEmpty()) continue;
        const int durationSec = item["duration_sec"] | 0;
        char duration[8];
        snprintf(duration, sizeof(duration), "%d:%02d", durationSec / 60, durationSec % 60);
        episodes.push_back({
            (const char*)(item["media_id"] | ""), title.c_str(),
            (const char*)(item["category"] | ""), duration, false,
            (const char*)(item["source_url"] | ""),
        });
    }
    return !episodes.empty();
}

// ─────────────────────────────────────────────────────────────────────────────
// UC4 — Companion chat
//
// POST /api/conversations/respond
// Request:  { "session_id": "<uuid>", "user_message": "..." }
// Response: { "conversation_id": "...", "safety_flag": "none",
//             "card": { "title": "...", "body": "...", "next_action": "..." } }
// ─────────────────────────────────────────────────────────────────────────────

bool EdgeApiClient::getCompanionReply(const String& sessionId,
                                      const String& message,
                                      String& reply) {
    // Use provided sessionId, fall back to last known session
    const String sid = sessionId.isEmpty() ? last_session_id_ : sessionId;
    if (sid.isEmpty()) {
        EDGE_LOG("getCompanionReply: no session_id available");
        return false;
    }

    JsonDocument req;
    req["session_id"]   = sid;
    req["user_message"] = message;

    String payload;
    serializeJson(req, payload);

    String response;
    if (!postJson("/api/conversations/respond", payload, response)) return false;

    JsonDocument json;
    if (deserializeJson(json, response) != DeserializationError::Ok) return false;

    reply = json["card"]["body"] | "";
    return !reply.isEmpty();
}

bool EdgeApiClient::canSync() const { return isReady(); }

bool EdgeApiClient::submitCompanionPcm(const String& sessionId, const char* pcmPath,
                                       String& transcript, String& reply, String& audioUrl) {
    if (!isReady() || sessionId.isEmpty() || pcmPath == nullptr) return false;
    File pcm = SPIFFS.open(pcmPath, FILE_READ);
    if (!pcm || pcm.size() == 0) {
        Serial.println("[Companion] PCM recording missing or empty");
        return false;
    }

    HTTPClient http;
    const String url = network_.serverBaseUrl() + "/api/conversations/voice?session_id=" + sessionId + "&sample_rate=16000";
    http.setTimeout(60000);  // Whisper + LLM + TTS may take longer than normal API calls.
    if (!http.begin(url)) {
        API_LOG("POST", "/api/conversations/voice", -1);
        pcm.close();
        return false;
    }
    http.addHeader("X-Device-Token", network_.deviceToken());
    http.addHeader("Content-Type", "application/octet-stream");
    http.addHeader("X-Audio-Format", "s16le");
    Serial.printf("[Companion] Uploading %u PCM bytes\n", (unsigned)pcm.size());
    const int status = http.sendRequest("POST", &pcm, pcm.size());
    API_LOG("POST", "/api/conversations/voice", status);
    pcm.close();
    if (status != HTTP_CODE_OK) {
        Serial.printf("[Companion] Voice API HTTP %d: %s\n", status, http.getString().c_str());
        http.end();
        return false;
    }

    const String body = http.getString();
    http.end();
    JsonDocument json;
    if (deserializeJson(json, body) != DeserializationError::Ok) return false;
    transcript = json["transcript"] | "";
    reply = json["reply_text"] | "";
    audioUrl = json["audio_path"] | "";
    if (!audioUrl.isEmpty() && audioUrl.startsWith("/")) audioUrl = network_.serverBaseUrl() + audioUrl;
    return !transcript.isEmpty() && !reply.isEmpty();
}

String EdgeApiClient::serverBaseUrl() const { return network_.serverBaseUrl(); }
String EdgeApiClient::deviceToken() const { return network_.deviceToken(); }

// ─────────────────────────────────────────────────────────────────────────────
// UC5 — Statistics
//
// GET /api/statistics/day   (period="Day")
// GET /api/statistics/week  (period="Week")
// GET /api/statistics/month (period="Month")
//
// Response (TftSummaryResponse):
//   {
//     "period_type": "daily",
//     "emotion_distribution": {
//       "angry": 0.10, "calm": 0.20, "disgust": 0.05, "fearful": 0.10,
//       "happy": 0.25, "neutral": 0.15, "sad": 0.10, "surprised": 0.05
//     },
//     ...
//   }
//
// NOTE: Values are decimals (0.0–1.0), NOT integer percentages.
//       Multiply by 100 before storing into uint8_t fields.
//       Embedded SER labels: angry, calm, disgust, fearful, happy, neutral,
//       sad and surprised.
// ─────────────────────────────────────────────────────────────────────────────

bool EdgeApiClient::getStatistics(const String& period,
                                  EmotionDistribution& dist) {
    // Map display period to API path segment
    String seg = period;
    seg.toLowerCase();
    String path;
    if      (seg == "day"   || seg == "daily")   path = "/api/statistics/day";
    else if (seg == "week"  || seg == "weekly")  path = "/api/statistics/week";
    else if (seg == "month" || seg == "monthly") path = "/api/statistics/month";
    else                                          path = "/api/statistics/week";

    String response;
    if (!getJson(path.c_str(), response)) return false;

    JsonDocument json;
    if (deserializeJson(json, response) != DeserializationError::Ok) {
        STATS_LOG("JSON parse failed for path=%s", path.c_str());
        return false;
    }

    JsonObject ed = json["emotion_distribution"].as<JsonObject>();
    if (ed.isNull()) {
        STATS_LOG("emotion_distribution key missing in response");
        return false;
    }
    if (ed.size() == 0) {
        STATS_LOG("emotion_distribution is empty — no sessions for this period, using mock");
        return false;
    }

    dist.period     = period.c_str();

    // Server stores values as decimals 0.0–1.0 (e.g. {"happy": 0.45}).
    // Multiply by 100 before casting to uint8_t to get correct percentages.
    dist.angryPct     = (uint8_t)(((float)(ed["angry"]     | 0.0f)) * 100.0f);
    dist.calmPct      = (uint8_t)(((float)(ed["calm"]      | 0.0f)) * 100.0f);
    dist.disgustPct   = (uint8_t)(((float)(ed["disgust"]   | 0.0f)) * 100.0f);
    dist.fearfulPct   = (uint8_t)(((float)(ed["fearful"]   | 0.0f)) * 100.0f);
    dist.happyPct     = (uint8_t)(((float)(ed["happy"]     | 0.0f)) * 100.0f);
    dist.neutralPct   = (uint8_t)(((float)(ed["neutral"]   | 0.0f)) * 100.0f);
    dist.sadPct       = (uint8_t)(((float)(ed["sad"]       | 0.0f)) * 100.0f);
    dist.surprisedPct = (uint8_t)(((float)(ed["surprised"] | 0.0f)) * 100.0f);

    EDGE_LOG("stats period=%s angry=%u calm=%u disgust=%u fearful=%u happy=%u neutral=%u sad=%u surprised=%u",
              dist.period.c_str(),
             dist.angryPct, dist.calmPct, dist.disgustPct, dist.fearfulPct,
             dist.happyPct, dist.neutralPct, dist.sadPct, dist.surprisedPct);
    return true;
}

bool EdgeApiClient::getStatisticsExplanation(const String& period,
                                             std::string& explanation) {
    String seg = period;
    seg.toLowerCase();
    if (seg == "daily") seg = "day";
    else if (seg == "weekly") seg = "week";
    else if (seg == "monthly") seg = "month";
    if (seg != "day" && seg != "week" && seg != "month") return false;

    String response;
    if (!postJson(("/api/statistics/" + seg + "/explain").c_str(), "{}", response)) {
        return false;
    }

    JsonDocument json;
    if (deserializeJson(json, response) != DeserializationError::Ok) return false;
    const char* text = json["explanation"] | "";
    if (!text || !*text) return false;
    explanation = text;
    return true;
}
