#pragma once
#include <TinyGPSPlus.h>
#include <LilyGoLib.h>

struct WiFiNetwork {
    const char *ssid;
    const char *password;
    bool hidden;
};

void time_sync_init(const WiFiNetwork *networks, int count);
void time_sync_loop(void);
bool time_sync_is_synced(void);
bool time_sync_wifi_connected(void);
void time_sync_force_retry(void);
