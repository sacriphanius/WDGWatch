#pragma once
#include <TinyGPSPlus.h>
#include <LilyGoLib.h>

// ============================================
// Haptic Feedback - DRV2605 integration
// ============================================

void haptic_init(void);
void haptic_click(void);         // Short click for button press
void haptic_double_click(void);  // Double tap
void haptic_buzz(void);          // Notification buzz
void haptic_alarm(void);         // Strong alarm vibration
void haptic_success(void);       // Success confirmation
void haptic_error(void);         // Error feedback
void haptic_play(uint8_t effect); // Play any effect 0-127
void haptic_set_enabled(bool en);
bool haptic_is_enabled(void);
