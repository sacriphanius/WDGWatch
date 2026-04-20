#pragma once

// ============================================
// T-Watch Ultra - Pip-Boy Firmware Config
// ============================================

// Display
#define SCREEN_WIDTH        410
#define SCREEN_HEIGHT       502
#define PIPBOY_MAX_BRIGHTNESS      255
#define PIPBOY_DEFAULT_BRIGHTNESS  128

// WatchDogsGo cyan color theme (#00E5FF)
#define PIPBOY_GREEN        0x00E5FF
#define PIPBOY_GREEN_DIM    0x007280
#define PIPBOY_GREEN_DARK   0x003840
#define PIPBOY_BG           0x000000

// LVGL 16-bit color equivalents
#define PIPBOY_GREEN_16     lv_color_hex(0x00E5FF)
#define PIPBOY_GREEN_DIM_16 lv_color_hex(0x007280)
#define PIPBOY_DARK_16      lv_color_hex(0x003840)
#define PIPBOY_BG_16        lv_color_hex(0x000000)

// Safe area - obudowa ścina ekran z każdej strony
#define SAFE_TOP    35
#define SAFE_BOTTOM 30
#define SAFE_LEFT   25
#define SAFE_RIGHT  25

// Sleep
#define SLEEP_TIMEOUT_MS    10000
#define DEEP_SLEEP_TIMEOUT  60000

// GPS
#define GPS_BAUD            38400

// LoRa defaults (SX1262)
#define LORA_FREQ           868.0
#define LORA_BW             125.0
#define LORA_SF             9
#define LORA_CR             7
#define LORA_POWER          14

// Vault-Boy animation
#define VAULTBOY_FRAMES     8
#define VAULTBOY_FPS        6
#define VAULTBOY_W          110
#define VAULTBOY_H          140

// App IDs
enum AppId {
    APP_WATCHFACE = 0,
    APP_MENU,
    APP_GPS,
    APP_LORA,
    APP_NFC,
    APP_SENSOR,
    APP_AUDIO,
    APP_RECON,
    APP_WIFI,
    APP_SETTINGS,
    APP_TOOLS,
    APP_COUNT
};
