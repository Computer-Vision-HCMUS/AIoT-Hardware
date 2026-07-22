/**
 * @file network_manager.cpp
 * @brief Wi-Fi provisioning + device pairing implementation.
 *
 * NVS key layout (namespace "aiot-net"):
 *   ssid         – Wi-Fi SSID
 *   pass         – Wi-Fi password
 *   server       – edge server base URL
 *   device_token – plain token returned by /api/devices/pair
 *   device_id    – UUID returned by /api/devices/pair
 *
 * Pairing flow (one-time, triggered from provisioning portal):
 *   1. User enters SSID + password + server URL + pairing code.
 *   2. Device connects to Wi-Fi.
 *   3. Device calls POST <server>/api/devices/pair.
 *   4. On 201 response: token + device_id saved to NVS → device restarts.
 *   5. On all subsequent boots: token loaded from NVS, no pairing needed.
 */

#include "network_manager.h"

#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <Preferences.h>
#include <WiFi.h>

namespace {
constexpr char kNamespace[]        = "aiot-net";
constexpr char kKeySSID[]          = "ssid";
constexpr char kKeyPass[]          = "pass";
constexpr char kKeyServer[]        = "server";
constexpr char kKeyDeviceToken[]   = "device_token";
constexpr char kKeyDeviceId[]      = "device_id";

constexpr char kApSsid[]           = "EmotiCare-Setup";
constexpr char kApPassword[]       = "emotioncare";
constexpr char kDeviceName[]       = "EmotiCare-ESP32";
constexpr char kFirmwareVersion[]  = "1.0.0";
constexpr unsigned long kReconnectEveryMs = 15000;

// Pairing endpoint (relative to server base URL)
constexpr char kPairPath[]         = "/api/devices/pair";
}  // namespace

// ─────────────────────────────────────────────────────────────────────────────
// Constructor
// ─────────────────────────────────────────────────────────────────────────────

NetworkManager::NetworkManager()
    : server_(80), provisioning_(false), last_reconnect_attempt_ms_(0) {}

// ─────────────────────────────────────────────────────────────────────────────
// Public interface
// ─────────────────────────────────────────────────────────────────────────────

bool NetworkManager::begin() {
    loadConfig();

    if (!connectSavedNetwork()) {
        startProvisioningPortal();
        return true;
    }

#if defined(DEV_PAIRING_CODE) && defined(DEV_SERVER_URL)
    // ── Dev auto-pair ───────────────────────────────────────────────────
    // If we just connected but have no token yet, pair automatically.
    if (!isPaired()) {
        Serial.println("[Network] DEV override: auto-pairing...");
        if (!pairDevice(DEV_SERVER_URL, DEV_PAIRING_CODE)) {
            Serial.println("[Network] DEV auto-pair failed — running unpaired");
        }
    }
#endif

    return true;  // offline UI stays usable
}

void NetworkManager::update() {
    if (provisioning_) {
        server_.handleClient();
        return;
    }
    if (WiFi.status() != WL_CONNECTED &&
        millis() - last_reconnect_attempt_ms_ >= kReconnectEveryMs) {
        last_reconnect_attempt_ms_ = millis();
        connectSavedNetwork();
    }
}

bool NetworkManager::isConnected()    const { return WiFi.status() == WL_CONNECTED; }
bool NetworkManager::isProvisioning() const { return provisioning_; }
bool NetworkManager::isPaired()       const { return !device_token_.isEmpty(); }

const String& NetworkManager::serverBaseUrl() const { return server_base_url_; }
const String& NetworkManager::deviceToken()   const { return device_token_; }
const String& NetworkManager::deviceId()      const { return device_id_; }

String NetworkManager::statusLabel() const {
    if (isConnected() && isPaired()) return "Online";
    if (isConnected())               return "Unpaired";
    if (provisioning_)               return "Setup AP";
    return "Offline";
}

// ─────────────────────────────────────────────────────────────────────────────
// Private — Wi-Fi
// ─────────────────────────────────────────────────────────────────────────────

bool NetworkManager::connectSavedNetwork() {
    Preferences prefs;
    prefs.begin(kNamespace, true);
    const String ssid = prefs.getString(kKeySSID, "");
    const String pass = prefs.getString(kKeyPass, "");
    prefs.end();

    if (ssid.isEmpty()) return false;

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid.c_str(), pass.c_str());

    const unsigned long started = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - started < 8000) {
        delay(100);
    }
    if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("[Network] Connected: %s  IP: %s\n",
                      ssid.c_str(), WiFi.localIP().toString().c_str());
        return true;
    }
    Serial.printf("[Network] Failed to connect to: %s\n", ssid.c_str());
    return false;
}

void NetworkManager::startProvisioningPortal() {
    WiFi.mode(WIFI_AP);
    WiFi.softAP(kApSsid, kApPassword);
    provisioning_ = true;
    Serial.printf("[Network] Setup AP: %s  open http://%s\n",
                  kApSsid, WiFi.softAPIP().toString().c_str());

    // GET / — show provisioning form
    server_.on("/", HTTP_GET, [this]() {
        server_.send(200, "text/html", portalPage());
    });

    // POST /save — save config, attempt pairing, restart
    server_.on("/save", HTTP_POST, [this]() {
        const String ssid       = server_.arg("ssid");
        const String password   = server_.arg("password");
        const String serverUrl  = server_.arg("server_url");
        const String pairingCode = server_.arg("pairing_code");

        if (ssid.isEmpty() || serverUrl.isEmpty() || pairingCode.isEmpty()) {
            server_.send(400, "text/plain",
                         "SSID, server URL and pairing code are required.");
            return;
        }

        // 1. Save Wi-Fi credentials first
        saveWifiConfig(ssid, password);

        // 2. Connect to the network
        WiFi.mode(WIFI_STA);
        WiFi.begin(ssid.c_str(), password.c_str());
        const unsigned long t0 = millis();
        while (WiFi.status() != WL_CONNECTED && millis() - t0 < 10000) {
            delay(200);
        }

        if (WiFi.status() != WL_CONNECTED) {
            server_.send(502, "text/plain",
                         "Could not connect to Wi-Fi. Check SSID/password.");
            return;
        }

        // 3. Attempt device pairing
        if (!pairDevice(serverUrl, pairingCode)) {
            server_.send(502, "text/plain",
                         "Wi-Fi OK but pairing failed. "
                         "Check server URL and pairing code.");
            return;
        }

        server_.send(200, "text/html",
                     "<!doctype html><html><body>"
                     "<h2>Paired successfully!</h2>"
                     "<p>Device will restart in 2 seconds.</p>"
                     "</body></html>");
        delay(2000);
        ESP.restart();
    });

    server_.begin();
}

// ─────────────────────────────────────────────────────────────────────────────
// Private — Pairing
// ─────────────────────────────────────────────────────────────────────────────

bool NetworkManager::pairDevice(const String& serverUrl,
                                const String& pairingCode) {
    const String url = serverUrl + kPairPath;

    // Build JSON body
    JsonDocument req;
    req["pairing_code"]      = pairingCode;
    req["device_name"]       = kDeviceName;
    req["firmware_version"]  = kFirmwareVersion;
    String payload;
    serializeJson(req, payload);

    HTTPClient http;
    http.setTimeout(8000);
    if (!http.begin(url)) {
        Serial.printf("[Network] pairDevice: http.begin failed for %s\n",
                      url.c_str());
        return false;
    }
    http.addHeader("Content-Type", "application/json");

    const int status = http.POST(payload);
    Serial.printf("[Network] POST %s → HTTP %d\n", url.c_str(), status);

    if (status != 201) {
        Serial.printf("[Network] Pairing failed (HTTP %d): %s\n",
                      status, http.getString().c_str());
        http.end();
        return false;
    }

    const String body = http.getString();
    http.end();

    JsonDocument resp;
    if (deserializeJson(resp, body) != DeserializationError::Ok) {
        Serial.println("[Network] Pairing: JSON parse error");
        return false;
    }

    const String token    = resp["device_token"] | "";
    const String deviceId = resp["device_id"]    | "";

    if (token.isEmpty() || deviceId.isEmpty()) {
        Serial.println("[Network] Pairing: missing token or device_id in response");
        return false;
    }

    saveServerConfig(serverUrl, token, deviceId);
    Serial.printf("[Network] Paired! device_id=%s\n", deviceId.c_str());
    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// Private — NVS
// ─────────────────────────────────────────────────────────────────────────────

void NetworkManager::loadConfig() {
    Preferences prefs;
    prefs.begin(kNamespace, true);
    server_base_url_ = prefs.getString(kKeyServer,      "");
    device_token_    = prefs.getString(kKeyDeviceToken, "");
    device_id_       = prefs.getString(kKeyDeviceId,    "");
    prefs.end();

#if defined(DEV_SERVER_URL)
    // ── Dev override: skip provisioning portal ──────────────────────────
    // Set DEV_WIFI_SSID / DEV_WIFI_PASS / DEV_SERVER_URL / DEV_PAIRING_CODE
    // in platformio.ini build_flags to connect + pair automatically.
    if (server_base_url_.isEmpty()) {
        Serial.println("[Network] DEV override: using hardcoded server URL");
        server_base_url_ = DEV_SERVER_URL;
    }
    if (device_token_.isEmpty()) {
        // Save credentials so connectSavedNetwork() can use them, then pair.
#if defined(DEV_WIFI_SSID) && defined(DEV_WIFI_PASS)
        saveWifiConfig(DEV_WIFI_SSID, DEV_WIFI_PASS);
#endif
    }
#endif
}

void NetworkManager::saveWifiConfig(const String& ssid, const String& password) {
    Preferences prefs;
    prefs.begin(kNamespace, false);
    prefs.putString(kKeySSID, ssid);
    prefs.putString(kKeyPass, password);
    prefs.end();
}

void NetworkManager::saveServerConfig(const String& serverUrl,
                                      const String& deviceToken,
                                      const String& deviceId) {
    Preferences prefs;
    prefs.begin(kNamespace, false);
    prefs.putString(kKeyServer,      serverUrl);
    prefs.putString(kKeyDeviceToken, deviceToken);
    prefs.putString(kKeyDeviceId,    deviceId);
    prefs.end();

    // Update in-memory copies immediately
    server_base_url_ = serverUrl;
    device_token_    = deviceToken;
    device_id_       = deviceId;
}

// ─────────────────────────────────────────────────────────────────────────────
// Private — Portal HTML
// ─────────────────────────────────────────────────────────────────────────────

String NetworkManager::portalPage() const {
    return F(
        "<!doctype html><html>"
        "<meta name='viewport' content='width=device-width,initial-scale=1'>"
        "<title>EmotiCare Setup</title>"
        "<style>body{font-family:sans-serif;max-width:420px;margin:40px auto;padding:0 16px}"
        "label{display:block;margin-top:12px;font-size:.9em;color:#555}"
        "input{width:100%;padding:8px;box-sizing:border-box;margin-top:4px}"
        "button{margin-top:20px;width:100%;padding:10px;background:#4CAF50;"
        "color:#fff;border:none;border-radius:4px;font-size:1em;cursor:pointer}"
        "</style><body>"
        "<h2>EmotiCare Setup</h2>"
        "<form method='post' action='/save'>"
        "<label>Wi-Fi SSID<input name='ssid' required></label>"
        "<label>Wi-Fi Password<input name='password' type='password'></label>"
        "<label>Server URL<input name='server_url' "
        "  placeholder='http://192.168.1.10:8000' required></label>"
        "<label>Pairing Code<input name='pairing_code' "
        "  placeholder='DEMO-001' required></label>"
        "<button type='submit'>Save &amp; Pair</button>"
        "</form></body></html>"
    );
}
