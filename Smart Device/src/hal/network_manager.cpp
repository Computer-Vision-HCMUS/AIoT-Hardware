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
constexpr char kApPassword[]       = "12345678";
const IPAddress kApIp(192, 168, 4, 1);
const IPAddress kApGateway(192, 168, 4, 1);
const IPAddress kApSubnet(255, 255, 255, 0);
constexpr char kDeviceName[]       = "EmotiCare-ESP32";
constexpr char kFirmwareVersion[]  = "1.0.0";
constexpr unsigned long kReconnectEveryMs = 15000;

// Pairing endpoint (relative to server base URL)
constexpr char kPairPath[]         = "/api/devices/pair";

String htmlEscape(const String& value) {
    String escaped;
    escaped.reserve(value.length());

    for (size_t i = 0; i < value.length(); ++i) {
        switch (value[i]) {
            case '&':  escaped += F("&amp;");  break;
            case '<':  escaped += F("&lt;");   break;
            case '>':  escaped += F("&gt;");   break;
            case '\"':  escaped += F("&quot;"); break;
            case '\'': escaped += F("&#39;");  break;
            default:   escaped += value[i];     break;
        }
    }
    return escaped;
}

String portalResultPage(const __FlashStringHelper* title,
                        const __FlashStringHelper* message,
                        const __FlashStringHelper* actionLabel) {
    String page = F(
        "<!doctype html><html><meta name='viewport' content='width=device-width,initial-scale=1'>"
        "<title>EmotiCare Setup</title>"
        "<style>body{font-family:sans-serif;max-width:420px;margin:40px auto;padding:0 16px;color:#18202b}"
        "a{display:block;margin-top:20px;padding:11px;background:#4CAF50;color:#fff;text-align:center;"
        "border-radius:4px;text-decoration:none}</style><body><h2>"
    );
    page += title;
    page += F("</h2><p>");
    page += message;
    page += F("</p><a href='/'>");
    page += actionLabel;
    page += F("</a></body></html>");
    return page;
}
}  // namespace

// ─────────────────────────────────────────────────────────────────────────────
// Constructor
// ─────────────────────────────────────────────────────────────────────────────

NetworkManager::NetworkManager()
    : server_(80), provisioning_(false),
      portal_routes_registered_(false),
      last_reconnect_attempt_ms_(0) {}

// ─────────────────────────────────────────────────────────────────────────────
// Public interface
// ─────────────────────────────────────────────────────────────────────────────

bool NetworkManager::begin() {
    loadConfig();

    if (!connectSavedNetwork()) {
        startProvisioningPortal();
    }
    return true;  // offline UI stays usable
}

void NetworkManager::update() {
    if (provisioning_) {
        // Call handleClient multiple times to avoid browser timeouts
        // during long TFT render cycles in the main loop
        for (int i = 0; i < 5; i++) {
            dns_server_.processNextRequest();
            server_.handleClient();
            delay(2);
        }
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
// WiFi mode control
// ─────────────────────────────────────────────────────────────────────────────

void NetworkManager::startProvisioningAp() {
    // Stop STA connection if active
    if (provisioning_) {
        // Already in AP mode — nothing to do
        Serial.println("[Network] Already in AP mode");
        return;
    }
    WiFi.disconnect(true);
    delay(200);
    startProvisioningPortal();
}

bool NetworkManager::reconnectWifi() {
    if (provisioning_) {
        // Tear down AP before connecting as STA
        server_.stop();
        provisioning_ = false;
        WiFi.softAPdisconnect(true);
        delay(200);
    }
    const bool ok = connectSavedNetwork();
    if (!ok) {
        // No saved credentials or network unreachable — go back to AP
        Serial.println("[Network] Reconnect failed — falling back to AP mode");
        startProvisioningPortal();
    }
    return ok;
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
    WiFi.disconnect(true);
    delay(100);
    WiFi.mode(WIFI_AP_STA);  // dual mode: AP stays alive while connecting to STA
    delay(100);
    if (!WiFi.softAPConfig(kApIp, kApGateway, kApSubnet) ||
        !WiFi.softAP(kApSsid, kApPassword)) {
        Serial.println("[Network] ERROR: failed to start setup AP");
        return;
    }
    delay(500);  // wait for AP to be fully ready before starting WebServer

    provisioning_ = true;
    Serial.printf("[Network] Setup AP: %s  open http://%s\n",
                  kApSsid, WiFi.softAPIP().toString().c_str());

    // ── Register routes only once — re-registering causes conflicts ───────
    if (!portal_routes_registered_) {
        portal_routes_registered_ = true;

        // Main setup form
        server_.on("/", HTTP_GET, [this]() {
            server_.send(200, "text/html", portalPage());
        });

        // Android captive portal probes
        server_.on("/generate_204", HTTP_GET, [this]() {
            server_.sendHeader("Location", "http://192.168.4.1/", true);
            server_.send(302, "text/plain", "");
        });
        server_.on("/gen_204", HTTP_GET, [this]() {
            server_.sendHeader("Location", "http://192.168.4.1/", true);
            server_.send(302, "text/plain", "");
        });

        // iOS / macOS captive portal probes
        server_.on("/hotspot-detect.html", HTTP_GET, [this]() {
            server_.sendHeader("Location", "http://192.168.4.1/", true);
            server_.send(302, "text/plain", "");
        });
        server_.on("/library/test/success.html", HTTP_GET, [this]() {
            server_.sendHeader("Location", "http://192.168.4.1/", true);
            server_.send(302, "text/plain", "");
        });

        // Windows NCSI probes
        server_.on("/ncsi.txt", HTTP_GET, [this]() {
            server_.sendHeader("Location", "http://192.168.4.1/", true);
            server_.send(302, "text/plain", "");
        });
        server_.on("/fwlink", HTTP_GET, [this]() {
            server_.sendHeader("Location", "http://192.168.4.1/", true);
            server_.send(302, "text/plain", "");
        });
        server_.on("/connecttest.txt", HTTP_GET, [this]() {
            server_.sendHeader("Location", "http://192.168.4.1/", true);
            server_.send(302, "text/plain", "");
        });

        // POST /save — save config, attempt pairing, restart
        server_.on("/save", HTTP_POST, [this]() {
            const String ssid        = server_.arg("ssid");
            const String password    = server_.arg("password");
            const String serverUrl   = server_.arg("server_url");
            const String pairingCode = server_.arg("pairing_code");

            if (ssid.isEmpty() || serverUrl.isEmpty() || pairingCode.isEmpty()) {
                server_.send(400, "text/html", portalResultPage(
                    F("Complete the setup form"),
                    F("Choose a Wi-Fi network, and enter both the server URL and pairing code."),
                    F("Back to setup")
                ));
                return;
            }

            // 1. Check the credentials before replacing the saved network.
            // Keeping the previous credentials lets the user recover from a
            // mistyped password without having to configure the device again.
            WiFi.begin(ssid.c_str(), password.c_str());
            const unsigned long t0 = millis();
            while (WiFi.status() != WL_CONNECTED && millis() - t0 < 10000) {
                server_.handleClient();  // keep AP alive during connect wait
                delay(100);
            }

            if (WiFi.status() != WL_CONNECTED) {
                server_.send(502, "text/html", portalResultPage(
                    F("Could not join Wi-Fi"),
                    F("Check the selected network and password, then try again. "
                      "Your previous Wi-Fi settings were kept."),
                    F("Try again")
                ));
                return;
            }

            // 2. The Wi-Fi credentials are valid, so it is now safe to save
            // them even if server pairing needs to be retried later.
            saveWifiConfig(ssid, password);

            // 3. Attempt device pairing
            if (!pairDevice(serverUrl, pairingCode)) {
                server_.send(502, "text/html", portalResultPage(
                    F("Wi-Fi connected, pairing failed"),
                    F("Check the server URL and pairing code, then try again. "
                      "The selected Wi-Fi was saved."),
                    F("Edit setup")
                ));
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

        // Catch-all: any unrecognised path → redirect to setup form
        server_.onNotFound([this]() {
            server_.sendHeader("Location", "http://192.168.4.1/", true);
            server_.send(302, "text/plain", "");
        });

    }
    dns_server_.start(53, "*", WiFi.softAPIP());
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
    String page = F(
        "<!doctype html><html>"
        "<meta name='viewport' content='width=device-width,initial-scale=1'>"
        "<title>EmotiCare Setup</title>"
        "<style>body{font-family:sans-serif;max-width:420px;margin:40px auto;padding:0 16px}"
        "label{display:block;margin-top:12px;font-size:.9em;color:#555}"
        "input,select{width:100%;padding:8px;box-sizing:border-box;margin-top:4px}"
        "button{margin-top:20px;width:100%;padding:10px;background:#4CAF50;"
        "color:#fff;border:none;border-radius:4px;font-size:1em;cursor:pointer}"
        ".secondary{margin-top:8px;background:#666}"
        ".hint{color:#555;line-height:1.45}.busy{display:none;margin-top:12px;color:#356b3a}"
        "</style><body>"
        "<h2>EmotiCare Setup</h2>"
        "<p class='hint'>Choose your home Wi-Fi, then enter its password. "
        "The device stays available at this page while it connects.</p>"
        "<form method='post' action='/save' onsubmit=\"document.getElementById('submit').disabled=true;"
        "document.getElementById('submit').textContent='Connecting...';"
        "document.getElementById('busy').style.display='block'\">"
        "<label>Wi-Fi network<select name='ssid' required>"
        "<option value='' selected disabled>Select a Wi-Fi network</option>"
    );

    const int networkCount = WiFi.scanNetworks(false, true);
    bool foundVisibleNetwork = false;
    for (int i = 0; i < networkCount; ++i) {
        const String ssid = WiFi.SSID(i);
        if (ssid.isEmpty()) continue;

        foundVisibleNetwork = true;
        const String safeSsid = htmlEscape(ssid);
        page += F("<option value='");
        page += safeSsid;
        page += F("'>");
        page += safeSsid;
        page += F(" (");
        page += WiFi.RSSI(i);
        page += F(" dBm)</option>");
    }
    WiFi.scanDelete();

    if (!foundVisibleNetwork) {
        page += F("<option value='' disabled>No Wi-Fi networks found. Refresh and try again.</option>");
    }

    page += F(
        "</select></label>"
        "<label>Wi-Fi Password<input name='password' type='password'></label>"
        "<label>Server URL<input name='server_url' "
        "  placeholder='http://192.168.1.10:8000' required></label>"
        "<label>Pairing Code<input name='pairing_code' "
        "  placeholder='DEMO-001' required></label>"
        "<button id='submit' type='submit'>Connect &amp; Pair</button>"
        "<p id='busy' class='busy'>Connecting to Wi-Fi and pairing. This can take about 20 seconds.</p>"
        "</form><button class='secondary' type='button' onclick='location.reload()'>Refresh Wi-Fi list</button>"
        "</body></html>"
    );
    return page;
}
