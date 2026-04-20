#pragma once
#include <LilyGoLib.h>

// ============================================
// Recon Service - WiFi/BLE scanning + deauth
// Background service, flag-based (safe from async web handlers)
// ============================================

struct ReconWiFi {
    char ssid[33];
    char bssid[18];
    int rssi;
    int channel;
    char auth[16];  // "OPEN", "WEP", "WPA", "WPA2", "WPA3", etc.
};

struct BleDevice {
    char mac[18];
    char name[33];
    int rssi;
    bool is_airtag;
};

void recon_service_init(void);
void recon_service_loop(void);  // call from main loop always

// Commands - safe to call from any context (web handler, UI, etc.)
// They set flags, actual work happens in recon_service_loop()
void recon_request_wifi_scan(void);
void recon_request_ble_scan(int duration_sec = 10);
void recon_request_deauth(const char* bssid, int channel);
void recon_request_deauth_all(void);           // blackout - all channels
void recon_request_sniffer(int channel = 0);   // promiscuous capture (0=hop)
void recon_request_deauth_detect(void);        // passive deauth monitor
void recon_request_evil_twin(const char* ssid, int channel);
void recon_request_stop(void);

// State queries
bool recon_is_scanning(void);
bool recon_is_deauthing(void);
bool recon_is_sniffing(void);
bool recon_is_evil_twin(void);

// Sniffer stats
int recon_sniffer_packet_count(void);
int recon_deauth_detect_count(void);

// Evil Twin
const char* recon_et_last_cred(void);
bool recon_et_has_new_cred(void);
int  recon_wifi_count(void);
int  recon_ble_count(void);

// Data access (valid until next scan)
const ReconWiFi* recon_get_wifi(int idx);
const BleDevice*   recon_get_ble(int idx);
