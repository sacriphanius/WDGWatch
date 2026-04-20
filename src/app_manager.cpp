#include <Arduino.h>
#include "app_manager.h"
#include "ui/watchface.h"
#include "ui/theme.h"
#include "apps/gps_app.h"
#include "apps/lora_app.h"
#include "apps/nfc_app.h"
#include "apps/sensor_app.h"
#include "apps/recon_app.h"
#include "apps/wifi_app.h"
#include "hal/haptic.h"

static AppId current_app = APP_WATCHFACE;
static lv_obj_t *scr_watchface = nullptr;
static lv_obj_t *scr_menu = nullptr;
static lv_obj_t *scr_app = nullptr;
static bool app_transitioning = false; // prevent double transitions

// Menu items (6 apps, 3x2 grid)
static const char *menu_items[] = {
    LV_SYMBOL_GPS " GPS",
    LV_SYMBOL_ENVELOPE " LoRa",
    LV_SYMBOL_SD_CARD " NFC",
    LV_SYMBOL_REFRESH " Sensor",
    LV_SYMBOL_EYE_OPEN " Recon",
    LV_SYMBOL_WIFI " WiFi",
};
static const AppId menu_app_ids[] = {
    APP_GPS, APP_LORA, APP_NFC, APP_SENSOR,
    APP_RECON, APP_WIFI,
};
static const int MENU_ITEM_COUNT = sizeof(menu_app_ids) / sizeof(menu_app_ids[0]);

// ---- Safely destroy current app ----
static void cleanup_app(void) {
    if (scr_app == nullptr) return;

    // Call app-specific destroy
    switch (current_app) {
        case APP_GPS:      gps_app_destroy(); break;
        case APP_LORA:     lora_app_destroy(); break;
        case APP_NFC:      nfc_app_destroy(); break;
        case APP_SENSOR:   sensor_app_destroy(); break;
        case APP_RECON:      recon_app_destroy(); break;
        case APP_WIFI:     wifi_app_destroy(); break;
        default: break;
    }

    // Delete the screen object
    lv_obj_delete(scr_app);
    scr_app = nullptr;
}

// ---- Create and show app ----
static void launch_app(AppId app) {
    // Always cleanup previous first
    cleanup_app();

    // Create fresh screen
    scr_app = lv_obj_create(nullptr);
    lv_obj_set_style_bg_color(scr_app, PIPBOY_BG_16, 0);
    lv_obj_set_style_bg_opa(scr_app, LV_OPA_COVER, 0);
    lv_obj_set_scrollbar_mode(scr_app, LV_SCROLLBAR_MODE_OFF);

    switch (app) {
        case APP_GPS:      gps_app_create(scr_app); break;
        case APP_LORA:     lora_app_create(scr_app); break;
        case APP_NFC:      nfc_app_create(scr_app); break;
        case APP_SENSOR:   sensor_app_create(scr_app); break;
        case APP_RECON:      recon_app_create(scr_app); break;
        case APP_WIFI:     wifi_app_create(scr_app); break;
        default: break;
    }

    // Swipe to go back
    lv_obj_add_event_cb(scr_app, [](lv_event_t *e) {
        lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_active());
        if (dir == LV_DIR_RIGHT || dir == LV_DIR_BOTTOM) {
            app_manager_back();
        }
    }, LV_EVENT_GESTURE, nullptr);

    // Load screen WITHOUT animation delete (we handle cleanup ourselves)
    lv_screen_load(scr_app);
    current_app = app;
    haptic_click();
}

// ---- Gesture on watchface ----
static void watchface_gesture_cb(lv_event_t *e) {
    lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_active());
    if (dir == LV_DIR_LEFT) {
        watchface_next();
        haptic_click();
    } else if (dir == LV_DIR_RIGHT) {
        watchface_prev();
        haptic_click();
    } else {
        app_manager_handle_gesture(dir);
    }
}

// ---- Menu button callback ----
static void menu_btn_cb(lv_event_t *e) {
    if (app_transitioning) return;
    AppId *id = (AppId *)lv_event_get_user_data(e);
    if (id) app_manager_show(*id);
}

// ---- Create app menu grid ----
static void create_menu_screen(void) {
    scr_menu = lv_obj_create(nullptr);
    lv_obj_set_style_bg_color(scr_menu, PIPBOY_BG_16, 0);
    lv_obj_set_style_bg_opa(scr_menu, LV_OPA_COVER, 0);
    lv_obj_set_scrollbar_mode(scr_menu, LV_SCROLLBAR_MODE_OFF);

    lv_obj_t *title = lv_label_create(scr_menu);
    lv_label_set_text(title, "[ PIPBOY 3000 ]");
    lv_obj_set_style_text_color(title, PIPBOY_GREEN_16, 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_16, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, SAFE_TOP);

    // Grid - 6 apps, 3x2
    const int BTN_W = 113;
    const int BTN_H = 170;

    lv_obj_t *grid = lv_obj_create(scr_menu);
    lv_obj_remove_style_all(grid);
    lv_obj_set_size(grid, SCREEN_WIDTH - SAFE_LEFT - SAFE_RIGHT, SCREEN_HEIGHT - SAFE_TOP - 25 - SAFE_BOTTOM);
    lv_obj_set_pos(grid, SAFE_LEFT, SAFE_TOP + 25);
    lv_obj_set_flex_flow(grid, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(grid, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_row(grid, 10, 0);
    lv_obj_set_style_pad_column(grid, 8, 0);
    lv_obj_set_scrollbar_mode(grid, LV_SCROLLBAR_MODE_OFF);

    static AppId btn_ids[6];

    for (int i = 0; i < MENU_ITEM_COUNT; i++) {
        btn_ids[i] = menu_app_ids[i];

        lv_obj_t *btn = lv_obj_create(grid);
        lv_obj_remove_style_all(btn);
        lv_obj_set_size(btn, BTN_W, BTN_H);
        lv_obj_set_style_bg_color(btn, PIPBOY_BG_16, 0);
        lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);
        lv_obj_set_style_border_color(btn, PIPBOY_GREEN_DIM_16, 0);
        lv_obj_set_style_border_width(btn, 1, 0);
        lv_obj_set_style_radius(btn, 0, 0);
        lv_obj_add_flag(btn, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_set_style_bg_color(btn, PIPBOY_GREEN_16, LV_STATE_PRESSED);
        lv_obj_set_style_bg_opa(btn, 50, LV_STATE_PRESSED);

        lv_obj_t *lbl = lv_label_create(btn);
        lv_label_set_text(lbl, menu_items[i]);
        lv_obj_set_style_text_color(lbl, PIPBOY_GREEN_16, 0);
        lv_obj_set_style_text_font(lbl, &lv_font_montserrat_16, 0);
        lv_obj_center(lbl);

        lv_obj_add_event_cb(btn, menu_btn_cb, LV_EVENT_CLICKED, &btn_ids[i]);
    }

    // Swipe down to go back
    lv_obj_add_event_cb(scr_menu, [](lv_event_t *e) {
        lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_active());
        if (dir == LV_DIR_BOTTOM) app_manager_show(APP_WATCHFACE);
    }, LV_EVENT_GESTURE, nullptr);
}

// ---- Public API ----

void app_manager_init(void) {
    pipboy_theme_init();

    scr_watchface = lv_obj_create(nullptr);
    lv_obj_set_style_bg_color(scr_watchface, PIPBOY_BG_16, 0);
    lv_obj_set_style_bg_opa(scr_watchface, LV_OPA_COVER, 0);
    lv_obj_set_scrollbar_mode(scr_watchface, LV_SCROLLBAR_MODE_OFF);

    watchface_create(scr_watchface);
    lv_obj_add_event_cb(scr_watchface, watchface_gesture_cb, LV_EVENT_GESTURE, nullptr);

    create_menu_screen();
    lv_screen_load(scr_watchface);
    current_app = APP_WATCHFACE;
}

void app_manager_show(AppId app) {
    if (app == current_app) return;
    if (app_transitioning) return;
    app_transitioning = true;

    // Always cleanup current app first
    if (current_app != APP_WATCHFACE && current_app != APP_MENU) {
        cleanup_app();
    }

    switch (app) {
        case APP_WATCHFACE:
            lv_screen_load(scr_watchface);
            current_app = APP_WATCHFACE;
            break;
        case APP_MENU:
            lv_screen_load(scr_menu);
            current_app = APP_MENU;
            break;
        default:
            launch_app(app);
            break;
    }

    app_transitioning = false;
}

AppId app_manager_current(void) { return current_app; }

void app_manager_back(void) {
    if (app_transitioning) return;
    if (current_app != APP_WATCHFACE && current_app != APP_MENU) {
        cleanup_app();
    }
    current_app = APP_WATCHFACE;
    lv_screen_load(scr_watchface);
}

void app_manager_handle_gesture(lv_dir_t dir) {
    if (current_app == APP_WATCHFACE && dir == LV_DIR_TOP) {
        app_manager_show(APP_MENU);
    } else if (current_app == APP_MENU && dir == LV_DIR_BOTTOM) {
        app_manager_show(APP_WATCHFACE);
    }
}

// Call from main loop to update active app (throttled)
void app_manager_update(void) {
    static uint32_t last_update = 0;
    uint32_t now = millis();
    if (now - last_update < 500) return;
    last_update = now;

    // Only update if we have a valid app screen
    if (scr_app == nullptr) return;

    switch (current_app) {
        case APP_GPS:    gps_app_update(); break;
        case APP_LORA:   lora_app_update(); break;
        case APP_NFC:    nfc_app_update(); break;
        case APP_SENSOR: sensor_app_update(); break;
        case APP_RECON:    recon_app_update(); break;
        case APP_WIFI:   wifi_app_update(); break;
        default: break;
    }
}
