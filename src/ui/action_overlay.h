#pragma once
#include <lvgl.h>

// Show action overlay - dims screen, shows status text, blocks touch
void action_overlay_show(const char *action_name);

// Update status text (e.g. "NFC Scan - TAG FOUND!")
void action_overlay_set_status(const char *status);

// Hide overlay, restore normal operation
void action_overlay_hide(void);

// Update time display
void action_overlay_set_time(const char *time_str);

// Is overlay active?
bool action_overlay_is_active(void);
