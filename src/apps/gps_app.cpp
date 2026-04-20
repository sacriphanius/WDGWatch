#include "app_common.h"
#include "gps_app.h"
#include <TinyGPSPlus.h>
#include <LilyGoLib.h>
#include <cstdio>
#include "../config.h"

static lv_obj_t *scr = nullptr;
static lv_obj_t *lbl_lat = nullptr;
static lv_obj_t *lbl_lon = nullptr;
static lv_obj_t *lbl_alt = nullptr;
static lv_obj_t *lbl_speed = nullptr;
static lv_obj_t *lbl_sats = nullptr;
static lv_obj_t *lbl_hdop = nullptr;
static lv_obj_t *lbl_heading = nullptr;
static lv_obj_t *lbl_fix = nullptr;

#define G lv_color_hex(0x00E5FF)
#define D lv_color_hex(0x007280)
#define BG lv_color_hex(0x000000)

static lv_obj_t* row(lv_obj_t *p, int y, const char *title) {
    lv_obj_t *lt = lv_label_create(p);
    lv_label_set_text(lt, title);
    lv_obj_set_style_text_color(lt, D, 0);
    lv_obj_set_style_text_font(lt, &lv_font_montserrat_14, 0);
    lv_obj_set_pos(lt, SAFE_LEFT + 10, y);

    lv_obj_t *lv = lv_label_create(p);
    lv_label_set_text(lv, "--");
    lv_obj_set_style_text_color(lv, G, 0);
    lv_obj_set_style_text_font(lv, &lv_font_montserrat_18, 0);
    lv_obj_set_pos(lv, SAFE_LEFT + 100, y - 2);
    return lv;
}

void gps_app_create(lv_obj_t *parent) {
    scr = lv_obj_create(parent);
    lv_obj_remove_style_all(scr);
    lv_obj_set_size(scr, 410, 502);
    lv_obj_set_style_bg_color(scr, BG, 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    lv_obj_center(scr);
    app_add_back_button(scr);

    lv_obj_t *title = lv_label_create(scr);
    lv_label_set_text(title, "[ GPS TRACKER ]");
    lv_obj_set_style_text_color(title, G, 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_18, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, SAFE_TOP);

    int y = SAFE_TOP + 30;
    lbl_fix    = row(scr, y, "FIX");       y += 35;
    lbl_sats   = row(scr, y, "SATS");      y += 35;
    lbl_lat    = row(scr, y, "LAT");       y += 35;
    lbl_lon    = row(scr, y, "LON");       y += 35;
    lbl_alt    = row(scr, y, "ALT");       y += 35;
    lbl_speed  = row(scr, y, "SPEED");     y += 35;
    lbl_heading= row(scr, y, "HDG");       y += 35;
    lbl_hdop   = row(scr, y, "HDOP");
}

void gps_app_update(void) {
    if (!scr || !lbl_fix) return;
    char b[32];

    if (instance.gps.location.isValid()) {
        lv_label_set_text(lbl_fix, "3D FIX");
        snprintf(b, sizeof(b), "%.6f", instance.gps.location.lat());
        lv_label_set_text(lbl_lat, b);
        snprintf(b, sizeof(b), "%.6f", instance.gps.location.lng());
        lv_label_set_text(lbl_lon, b);
    } else {
        lv_label_set_text(lbl_fix, "NO FIX");
        lv_label_set_text(lbl_lat, "--");
        lv_label_set_text(lbl_lon, "--");
    }

    if (instance.gps.altitude.isValid()) {
        snprintf(b, sizeof(b), "%.1f m", instance.gps.altitude.meters());
        lv_label_set_text(lbl_alt, b);
    }
    if (instance.gps.speed.isValid()) {
        snprintf(b, sizeof(b), "%.1f km/h", instance.gps.speed.kmph());
        lv_label_set_text(lbl_speed, b);
    }
    if (instance.gps.course.isValid()) {
        snprintf(b, sizeof(b), "%.1f°", instance.gps.course.deg());
        lv_label_set_text(lbl_heading, b);
    }
    snprintf(b, sizeof(b), "%d", instance.gps.satellites.value());
    lv_label_set_text(lbl_sats, b);
    snprintf(b, sizeof(b), "%.1f", instance.gps.hdop.hdop());
    lv_label_set_text(lbl_hdop, b);
}

void gps_app_destroy(void) {
    if (scr) { lv_obj_delete(scr); scr = nullptr; }
    lbl_lat = lbl_lon = lbl_alt = lbl_speed = nullptr;
    lbl_sats = lbl_hdop = lbl_heading = lbl_fix = nullptr;
}
