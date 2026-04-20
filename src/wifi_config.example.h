#pragma once
// Copy this file to `src/wifi_config.h` and fill in your WiFi networks.
// `wifi_config.h` is gitignored — keep credentials out of the repo.
//
// Networks are tried in order until one connects. Only used for NTP time sync;
// the watch does NOT stay connected to the internet after sync completes.

#include "hal/time_sync.h"

static const WiFiNetwork wifi_networks[] = {
    {"YourSSID",       "YourPassword",       false},  // regular open/WPA2 AP
    {"HiddenSSID",     "HiddenPassword",     true},   // hidden AP
};
