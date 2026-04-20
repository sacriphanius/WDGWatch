#pragma once
#include <lvgl.h>
#include "../config.h"

void statusbar_create(lv_obj_t *parent);
void statusbar_update_battery(uint8_t percent, bool charging);
void statusbar_update_ble(bool connected);
void statusbar_update_wifi(bool connected);
void statusbar_update_gps(bool fix);
void statusbar_update_lora(bool active);
