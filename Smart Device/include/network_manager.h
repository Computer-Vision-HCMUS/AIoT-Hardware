#ifndef NETWORK_MANAGER_H
#define NETWORK_MANAGER_H

#include <Arduino.h>
#include <DNSServer.h>
#include <WebServer.h>

/**
 * @file network_manager.h
 * @brief Wi-Fi provisioning + device pairing for EmotiCare.
 *
 * Responsibilities:
 *  1. Connect to a saved Wi-Fi network (credentials in NVS).
 *  2. If no saved network, start a captive-portal AP for provisioning.
 *  3. During provisioning the user also enters:
 *       - Edge server base URL  (e.g. http://192.168.1.10:8000)
 *       - Pairing code          (e.g. DEMO-001)
 *     The device then calls POST /api/devices/pair and stores the returned
 *     device_token in NVS.  All subsequent API calls send that token via the
 *     X-Device-Token header.
 *
 * Thread safety: not thread-safe — call from a single Arduino task only.
 */
class NetworkManager {
public:
    NetworkManager();

    /** Must be called once in setup(). Returns true always (offline is OK). */
    bool begin();

    /** Must be called every loop() iteration. */
    void update();

    // ── WiFi mode control ─────────────────────────────────────────────────
    /** Disconnect from WiFi and start AP provisioning portal. */
    void startProvisioningAp();

    /** Connect using saved NVS credentials (if any). */
    bool reconnectWifi();

    // ── Status ────────────────────────────────────────────────────────────
    bool   isConnected()    const;
    bool   isProvisioning() const;
    bool   isPaired()       const;   ///< true once device_token is stored
    String statusLabel()    const;

    // ── Server / token accessors ──────────────────────────────────────────
    /** Base URL without trailing slash, e.g. "http://192.168.1.10:8000". */
    const String& serverBaseUrl() const;

    /** Plain device_token to be sent as X-Device-Token header. */
    const String& deviceToken()   const;

    /** device_id returned by /api/devices/pair (UUID). */
    const String& deviceId()      const;

private:
    // ── Networking ────────────────────────────────────────────────────────
    bool   connectSavedNetwork();
    void   startProvisioningPortal();
    String portalPage() const;

    // ── Pairing ───────────────────────────────────────────────────────────
    /**
     * Calls POST /api/devices/pair with the stored pairing_code.
     * On success stores device_token + device_id to NVS and returns true.
     */
    bool   pairDevice(const String& serverUrl, const String& pairingCode);

    // ── NVS persistence ───────────────────────────────────────────────────
    void saveWifiConfig(const String& ssid, const String& password);
    void saveServerConfig(const String& serverUrl, const String& deviceToken,
                          const String& deviceId);
    void loadConfig();

    // ── Internal state ────────────────────────────────────────────────────
    WebServer server_;
    DNSServer dns_server_;
    String    server_base_url_;
    String    device_token_;
    String    device_id_;
    bool      provisioning_;
    bool      portal_routes_registered_;
    unsigned long last_reconnect_attempt_ms_;
};

#endif  // NETWORK_MANAGER_H
