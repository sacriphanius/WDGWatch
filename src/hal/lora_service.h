#pragma once
#include <LilyGoLib.h>

// MeshCore message received callback
struct MeshMsg {
    char channel[32];
    char text[200];
    float rssi;
    int hops;
    uint32_t timestamp;
};

struct MeshNode {
    char name[32];
    char type[16];
    float lat, lon;
    float rssi, snr;
    uint8_t pubkey[32];
};

void lora_service_init(void);
void lora_service_loop(void);  // call from main loop always

// Commands
void lora_svc_start(void);    // start MeshCore RX
void lora_svc_stop(void);
void lora_svc_send_message(const char *text);
void lora_svc_send_advert(void);
void lora_svc_set_node_name(const char *name);

// State
bool lora_svc_is_running(void);
bool lora_svc_has_new_message(void);      // for watch UI (returns true once)
bool lora_svc_has_new_message_web(void); // for web UI (separate flag)
bool lora_svc_has_new_message_ble(void); // for BLE/api_loop (separate flag)
const MeshMsg* lora_svc_last_message(void);
const MeshMsg* lora_svc_get_message(int idx);  // 0=newest
int lora_svc_message_count(void);
int lora_svc_node_count(void);
void lora_svc_save_history(void);
void lora_svc_load_history(void);
