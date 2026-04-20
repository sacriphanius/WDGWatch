#pragma once
#include <lvgl.h>
#include "config.h"

// ============================================
// App Manager - Screen navigation & lifecycle
// ============================================

void app_manager_init(void);
void app_manager_show(AppId app);
AppId app_manager_current(void);
void app_manager_back(void);

// Gesture handling
void app_manager_handle_gesture(lv_dir_t dir);

// Call from main loop to update active app
void app_manager_update(void);
