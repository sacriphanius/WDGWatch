#pragma once
#include <Arduino.h>

// Unified command handler - same commands for BLE and WiFi
// Input: JSON command string
// Output: JSON response string (caller must free with free())
// Events: pushed via callback

typedef void (*api_event_cb_t)(const char *json_event);

void api_init(void);
char* api_handle_command(const char *json_cmd); // caller must free() result
void api_set_event_callback(api_event_cb_t cb);
void api_loop(void); // check for pending events to push
