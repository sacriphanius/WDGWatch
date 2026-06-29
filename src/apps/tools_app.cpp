#include "tools_app.h"
#include <LilyGoLib.h>
#include <cstdio>
#include <cmath>
#include "../config.h"
#include "../hal/haptic.h"
#include "../hal/sound_settings.h"
#include "app_common.h"

static lv_obj_t *scr = nullptr;

#define G  lv_color_hex(PIPBOY_GREEN)
#define D  lv_color_hex(PIPBOY_GREEN_DIM)
#define BG lv_color_hex(PIPBOY_BG)

#define SA_X      SAFE_LEFT
#define SA_Y      SAFE_TOP
#define SA_W      (SCREEN_WIDTH  - SAFE_LEFT - SAFE_RIGHT)
#define SA_H      (SCREEN_HEIGHT - SAFE_TOP  - SAFE_BOTTOM)

static lv_obj_t  *lbl_stopwatch   = nullptr;
static uint32_t   sw_start        = 0;
static uint32_t   sw_elapsed      = 0;
static bool       sw_running      = false;
static lv_timer_t *sw_timer       = nullptr;

static lv_obj_t  *lbl_timer       = nullptr;
static lv_obj_t  *lbl_timer_status = nullptr;
static lv_obj_t  *btn_timer_start = nullptr;
static lv_obj_t  *btn_timer_reset = nullptr;
static lv_timer_t *ct_timer       = nullptr;
static uint32_t   ct_duration_ms  = 60000;
static uint32_t   ct_start        = 0;
static bool       ct_running      = false;
static bool       ct_finished     = false;

static int        flashlight_state = 0;
static lv_timer_t *sos_timer       = nullptr;
static int        sos_step         = 0;
static lv_obj_t   *fl_btn_lbl      = nullptr;

static bool       repellent_on     = false;
static TaskHandle_t repellent_task_handle = nullptr;
static lv_obj_t   *rep_btn_lbl     = nullptr;

static const uint32_t ct_presets[] = { 1, 3, 5, 10, 15 };
#define CT_PRESET_COUNT 5
static int ct_preset_idx = 0;

static lv_obj_t* make_btn(lv_obj_t *par, int x, int y, int w, int h,
                           const char *txt, lv_event_cb_t cb) {
    lv_obj_t *btn = lv_button_create(par);
    lv_obj_set_size(btn, w, h);
    lv_obj_set_pos(btn, x, y);
    lv_obj_set_style_bg_color(btn, BG, 0);
    lv_obj_set_style_border_color(btn, G, 0);
    lv_obj_set_style_border_width(btn, 1, 0);
    style_button_by_position(btn, y, h);
    lv_obj_set_style_pad_all(btn, 0, 0);
    lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, nullptr);
    lv_obj_t *l = lv_label_create(btn);
    lv_label_set_text(l, txt);
    lv_obj_set_style_text_color(l, G, 0);
    lv_obj_set_style_text_font(l, &lv_font_montserrat_18, 0);
    lv_obj_center(l);
    return btn;
}

static lv_obj_t* make_section_label(lv_obj_t *par, const char *txt) {
    lv_obj_t *l = lv_label_create(par);
    lv_label_set_text(l, txt);
    lv_obj_set_style_text_color(l, D, 0);
    lv_obj_set_style_text_font(l, &lv_font_montserrat_18, 0);
    return l;
}

static void format_mmss(char *buf, size_t sz, uint32_t total_ms) {
    uint32_t secs = total_ms / 1000;
    uint32_t m = secs / 60;
    uint32_t s = secs % 60;
    snprintf(buf, sz, "%02lu:%02lu", (unsigned long)m, (unsigned long)s);
}

const uint32_t sos_pattern[] = {
    150, 150, 150, 150, 150, 300,
    450, 150, 450, 150, 450, 300,
    150, 150, 150, 150, 150, 1000
};

static void sos_timer_cb(lv_timer_t *t) {
    (void)t;
    bool is_on = (sos_step % 2 == 0);
    if (is_on) {
        instance.setBrightness(PIPBOY_MAX_BRIGHTNESS);
        lv_obj_set_style_bg_color(scr, lv_color_hex(0xFFFFFF), 0);
    } else {
        instance.setBrightness(0);
        lv_obj_set_style_bg_color(scr, BG, 0);
    }
    uint32_t next_dur = sos_pattern[sos_step];
    lv_timer_set_period(t, next_dur);
    sos_step = (sos_step + 1) % 18;
}

static void flashlight_cb(lv_event_t *e) {
    (void)e;
    flashlight_state = (flashlight_state + 1) % 3;
    if (sos_timer) {
        lv_timer_delete(sos_timer);
        sos_timer = nullptr;
    }
    if (flashlight_state == 1) {
        if (fl_btn_lbl) lv_label_set_text(fl_btn_lbl, "LIGHT: STEADY");
        instance.setBrightness(PIPBOY_MAX_BRIGHTNESS);
        lv_obj_set_style_bg_color(scr, lv_color_hex(0xFFFFFF), 0);
    } else if (flashlight_state == 2) {
        if (fl_btn_lbl) lv_label_set_text(fl_btn_lbl, "LIGHT: SOS");
        sos_step = 0;
        sos_timer = lv_timer_create(sos_timer_cb, 10, nullptr);
    } else {
        if (fl_btn_lbl) lv_label_set_text(fl_btn_lbl, "LIGHT: OFF");
        instance.setBrightness(PIPBOY_DEFAULT_BRIGHTNESS);
        lv_obj_set_style_bg_color(scr, BG, 0);
    }
}

static void repellent_task(void *pvParameters) {
    (void)pvParameters;
    instance.powerControl(POWER_SPEAK, true);
    instance.player.configureTX(44100, I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO);

    const float f_s = 19000.0f;
    const float f_e = 21800.0f;
    const float T_sweep = 0.1f;
    const float pi = 3.14159265f;
    const int sample_rate = 44100;
    const int chunk_samples = 512;
    int16_t buf[chunk_samples * 2];

    int sample_idx = 0;
    int sweep_samples = (int)(T_sweep * sample_rate);

    while (repellent_on) {
        float vol_factor = (float)sound_get_volume() / 100.0f;
        if (vol_factor < 0.1f) vol_factor = 0.8f;

        for (int i = 0; i < chunk_samples; i++) {
            float t = (float)(sample_idx % sweep_samples) / sample_rate;
            float phase = 2.0f * pi * (f_s * t + 0.5f * (f_e - f_s) * t * t / T_sweep);
            float s = sinf(phase);
            int16_t val = (int16_t)((s >= 0.0f ? 1.0f : -1.0f) * 32000.0f * vol_factor);
            buf[i * 2]     = val;
            buf[i * 2 + 1] = val;
            sample_idx++;
        }

        instance.player.write((const uint8_t*)buf, chunk_samples * 4);
        vTaskDelay(pdMS_TO_TICKS(8));
    }

    instance.powerControl(POWER_SPEAK, false);
    repellent_task_handle = nullptr;
    vTaskDelete(nullptr);
}

static void repellent_cb(lv_event_t *e) {
    (void)e;
    repellent_on = !repellent_on;
    if (repellent_on) {
        if (rep_btn_lbl) lv_label_set_text(rep_btn_lbl, "REPELLER: ON");
        xTaskCreatePinnedToCore(repellent_task, "repellent_task", 4096, nullptr, 5, &repellent_task_handle, 0);
    } else {
        if (rep_btn_lbl) lv_label_set_text(rep_btn_lbl, "REPELLER: OFF");
    }
}

static void sw_update_label(void) {
    if (!lbl_stopwatch) return;
    uint32_t elapsed = sw_elapsed;
    if (sw_running) elapsed += millis() - sw_start;
    uint32_t mins = elapsed / 60000;
    uint32_t secs = (elapsed / 1000) % 60;
    uint32_t cs   = (elapsed / 10) % 100;
    char b[16];
    snprintf(b, sizeof(b), "%02lu:%02lu.%02lu",
             (unsigned long)mins, (unsigned long)secs, (unsigned long)cs);
    lv_label_set_text(lbl_stopwatch, b);
}

static void sw_tick(lv_timer_t *t) {
    (void)t;
    if (sw_running) sw_update_label();
}

static void stopwatch_cb(lv_event_t *e) {
    (void)e;
    if (sw_running) {
        sw_elapsed += millis() - sw_start;
        sw_running = false;
    } else {
        sw_start = millis();
        sw_running = true;
    }
}

static void stopwatch_reset_cb(lv_event_t *e) {
    (void)e;
    sw_running = false;
    sw_start   = 0;
    sw_elapsed = 0;
    if (lbl_stopwatch) lv_label_set_text(lbl_stopwatch, "00:00.00");
}

static void ct_update_preset_label(void) {
    if (!lbl_timer) return;
    char b[16];
    format_mmss(b, sizeof(b), ct_presets[ct_preset_idx] * 60000UL);
    lv_label_set_text(lbl_timer, b);
}

static void ct_update_running_label(void) {
    if (!lbl_timer) return;
    uint32_t elapsed = millis() - ct_start;
    if (elapsed >= ct_duration_ms) {
        lv_label_set_text(lbl_timer, "00:00");
        return;
    }
    uint32_t remain = ct_duration_ms - elapsed;
    char b[16];
    format_mmss(b, sizeof(b), remain);
    lv_label_set_text(lbl_timer, b);
}

static void ct_tick(lv_timer_t *t) {
    (void)t;
    if (!ct_running) return;
    uint32_t elapsed = millis() - ct_start;
    if (elapsed >= ct_duration_ms) {
        ct_running  = false;
        ct_finished = true;
        lv_label_set_text(lbl_timer, "00:00");
        if (lbl_timer_status) lv_label_set_text(lbl_timer_status, "DONE!");
        haptic_alarm();
    } else {
        ct_update_running_label();
    }
}

static void ct_preset_cb(lv_event_t *e) {
    (void)e;
    if (ct_running) return;
    ct_preset_idx = (ct_preset_idx + 1) % CT_PRESET_COUNT;
    ct_duration_ms = ct_presets[ct_preset_idx] * 60000UL;
    ct_finished = false;
    ct_update_preset_label();
    if (lbl_timer_status) lv_label_set_text(lbl_timer_status, "");
}

static void ct_start_cb(lv_event_t *e) {
    (void)e;
    if (ct_running) {

        ct_running = false;
        if (lbl_timer_status) lv_label_set_text(lbl_timer_status, "PAUSED");
    } else if (!ct_finished) {
        ct_start   = millis();
        ct_running = true;
        if (lbl_timer_status) lv_label_set_text(lbl_timer_status, "RUNNING");
    }
}

static void ct_reset_cb(lv_event_t *e) {
    (void)e;
    ct_running  = false;
    ct_finished = false;
    ct_update_preset_label();
    if (lbl_timer_status) lv_label_set_text(lbl_timer_status, "");
}

void tools_app_create(lv_obj_t *parent) {

    scr = lv_obj_create(parent);
    lv_obj_remove_style_all(scr);
    lv_obj_set_pos(scr, SA_X, SA_Y);
    lv_obj_set_size(scr, SA_W, SA_H);
    lv_obj_set_style_bg_color(scr, BG, 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all(scr, 0, 0);
    lv_obj_set_style_pad_row(scr, 0, 0);
    lv_obj_set_style_pad_column(scr, 0, 0);
    lv_obj_set_scrollbar_mode(scr, LV_SCROLLBAR_MODE_AUTO);
    lv_obj_add_flag(scr, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scroll_dir(scr, LV_DIR_VER);

    const int CW = SA_W;

    int y = 0;
    lv_obj_set_style_pad_bottom(scr, 40, 0);

    lv_obj_t *title = lv_label_create(scr);
    lv_label_set_text(title, "[ TOOLS ]");
    lv_obj_set_style_text_color(title, G, 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_22, 0);
    lv_obj_set_pos(title, CW / 2 - 50, y);
    y += 26;

    lv_obj_t *fl_lbl = make_section_label(scr, "FLASHLIGHT & REPELLENT");
    lv_obj_set_pos(fl_lbl, 0, y);
    y += 18;

    lv_obj_t *fl_btn = make_btn(scr, 0, y, CW / 2 - 5, 44, "LIGHT: OFF", flashlight_cb);
    fl_btn_lbl = lv_obj_get_child(fl_btn, 0);
    lv_obj_set_style_text_font(fl_btn_lbl, &lv_font_montserrat_14, 0);

    lv_obj_t *rep_btn = make_btn(scr, CW / 2 + 5, y, CW / 2 - 5, 44, "REPELLER: OFF", repellent_cb);
    rep_btn_lbl = lv_obj_get_child(rep_btn, 0);
    lv_obj_set_style_text_font(rep_btn_lbl, &lv_font_montserrat_14, 0);

    y += 50;

    lv_obj_t *sw_sec = make_section_label(scr, "STOPWATCH");
    lv_obj_set_pos(sw_sec, 0, y);
    y += 18;

    lbl_stopwatch = lv_label_create(scr);
    lv_label_set_text(lbl_stopwatch, "00:00.00");
    lv_obj_set_style_text_color(lbl_stopwatch, G, 0);
    lv_obj_set_style_text_font(lbl_stopwatch, &lv_font_montserrat_40, 0);
    lv_obj_set_pos(lbl_stopwatch, 0, y);
    y += 46;

    lv_obj_t *sw_btn1 = make_btn(scr, 0, y, 140, 44, "START / STOP", stopwatch_cb);
    lv_obj_t *sw_btn2 = make_btn(scr, 150, y, 100, 44, "RESET", stopwatch_reset_cb);
    y += 50;

    lv_obj_t *ct_sec = make_section_label(scr, "COUNTDOWN TIMER");
    lv_obj_set_pos(ct_sec, 0, y);
    y += 18;

    lv_obj_t *preset_btn = make_btn(scr, 0, y, 120, 44, "SET TIME", ct_preset_cb);
    y += 50;

    lbl_timer = lv_label_create(scr);
    lv_obj_set_style_text_color(lbl_timer, G, 0);
    lv_obj_set_style_text_font(lbl_timer, &lv_font_montserrat_40, 0);
    lv_obj_set_pos(lbl_timer, 0, y);
    ct_update_preset_label();
    y += 46;

    lbl_timer_status = lv_label_create(scr);
    lv_label_set_text(lbl_timer_status, "");
    lv_obj_set_style_text_color(lbl_timer_status, D, 0);
    lv_obj_set_style_text_font(lbl_timer_status, &lv_font_montserrat_18, 0);
    lv_obj_set_pos(lbl_timer_status, 0, y);
    y += 18;

    btn_timer_start = make_btn(scr, 0, y, 140, 44, "START / STOP", ct_start_cb);
    btn_timer_reset = make_btn(scr, 150, y, 100, 44, "RESET", ct_reset_cb);
    y += 50;

    sw_timer = lv_timer_create(sw_tick, 50, nullptr);
    ct_timer = lv_timer_create(ct_tick, 250, nullptr);
}

void tools_app_destroy(void) {
    if (sos_timer) {
        lv_timer_delete(sos_timer);
        sos_timer = nullptr;
    }
    if (flashlight_state != 0) {
        instance.setBrightness(PIPBOY_DEFAULT_BRIGHTNESS);
        flashlight_state = 0;
    }
    if (repellent_on) {
        repellent_on = false;
    }
    repellent_task_handle = nullptr;

    sw_running  = false;
    sw_elapsed  = 0;
    ct_running  = false;
    ct_finished = false;
    if (sw_timer) { lv_timer_delete(sw_timer); sw_timer = nullptr; }
    if (ct_timer) { lv_timer_delete(ct_timer); ct_timer = nullptr; }
    lbl_stopwatch    = nullptr;
    lbl_timer        = nullptr;
    lbl_timer_status = nullptr;
    btn_timer_start  = nullptr;
    btn_timer_reset  = nullptr;
    fl_btn_lbl       = nullptr;
    rep_btn_lbl      = nullptr;
    if (scr) { lv_obj_delete(scr); scr = nullptr; }
}
