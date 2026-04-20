#pragma once
#include <LilyGoLib.h>

// Background NFC service - runs independently of UI
void nfc_service_init(void);
void nfc_service_loop(void);  // call from main loop always

// Commands - safe to call from any context (web handler, UI, etc.)
// They set flags, actual work happens in nfc_service_loop()
void nfc_svc_request_scan(void);
void nfc_svc_request_stop(void);
void nfc_svc_request_save(void);
void nfc_svc_request_delete(int idx);
void nfc_svc_request_export(void);
void nfc_svc_request_select_next(void);
void nfc_svc_request_emulate(void);

// State queries
bool nfc_svc_is_scanning(void);
bool nfc_svc_is_emulating(void);
const char* nfc_svc_last_uid(void);
const char* nfc_svc_last_ndef(void);
int nfc_svc_saved_count(void);
int nfc_svc_selected_idx(void);
const char* nfc_svc_tag_name(int idx);
bool nfc_svc_tag_detected(void);      // watch UI consumer
bool nfc_svc_tag_detected_web(void);  // web server consumer
bool nfc_svc_tag_detected_ble(void);  // BLE / api consumer
