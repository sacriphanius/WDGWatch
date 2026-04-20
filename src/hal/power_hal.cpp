#include "power_hal.h"

static uint32_t last_activity_ms = 0;
static uint32_t sleep_timeout_ms = SLEEP_TIMEOUT_MS;
static bool screen_off = false;

void power_hal_init(void) {
    last_activity_ms = millis();

    // Register PMU event handler
    instance.onEvent([](DeviceEvent_t event, void *params, void *user_data) {
        if (event == POWER_EVENT) {
            PMUEventType_t pmu_event = instance.getPMUEventType(params);
            if (pmu_event == PMU_EVENT_KEY_CLICKED) {
                power_hal_reset_activity();
            }
        }
    });
}

void power_hal_check_sleep(void) {
    if (screen_off) return;
    uint32_t elapsed = millis() - last_activity_ms;
    if (elapsed > sleep_timeout_ms) {
        power_hal_screen_toggle(); // Turn off screen
    }
}

void power_hal_reset_activity(void) {
    last_activity_ms = millis();
}

void power_hal_light_sleep(void) {
    instance.setBrightness(0);

    // Disable non-essential peripherals
    instance.powerControl(POWER_GPS, false);
    instance.powerControl(POWER_NFC, false);

    // Enter light sleep - wake from power key + touch
    instance.lightSleep((WakeupSource_t)(WAKEUP_SRC_POWER_KEY | WAKEUP_SRC_TOUCH_PANEL));

    // Restore on wake
    instance.powerControl(POWER_GPS, true);
    instance.setBrightness(PIPBOY_DEFAULT_BRIGHTNESS);
    power_hal_reset_activity();
}

void power_hal_deep_sleep(void) {
    instance.setBrightness(0);
    instance.powerControl(POWER_GPS, false);
    instance.powerControl(POWER_NFC, false);
    instance.powerControl(POWER_RADIO, false);
    instance.powerControl(POWER_SPEAK, false);

    instance.sleep((WakeupSource_t)(WAKEUP_SRC_POWER_KEY | WAKEUP_SRC_BOOT_BUTTON));

    power_hal_reset_activity();
}

// Cached PMU values (updated every 2s, not on every call)
static uint8_t cached_bat_pct = 0;
static bool cached_charging = false;
static float cached_bat_volt = 0;
static uint32_t last_pmu_read = 0;

static void refresh_pmu_cache(void) {
    uint32_t now = millis();
    if (now - last_pmu_read < 2000) return;
    last_pmu_read = now;
    int pct = instance.pmu.getBatteryPercent();
    cached_bat_pct = (pct < 0) ? 0 : (pct > 100) ? 100 : (uint8_t)pct;
    cached_charging = instance.pmu.isCharging();
    cached_bat_volt = instance.pmu.getBattVoltage() / 1000.0f;
}

uint8_t power_hal_battery_percent(void) {
    refresh_pmu_cache();
    return cached_bat_pct;
}

bool power_hal_is_charging(void) {
    refresh_pmu_cache();
    return cached_charging;
}

float power_hal_battery_voltage(void) {
    refresh_pmu_cache();
    return cached_bat_volt;
}

static PowerProfile current_profile = PROFILE_BALANCED;

void power_hal_set_profile(PowerProfile profile) {
    current_profile = profile;
    switch (profile) {
        case PROFILE_PERFORMANCE:
            sleep_timeout_ms = 30000;  // 30s
            setCpuFrequencyMhz(240);
            break;
        case PROFILE_BALANCED:
            sleep_timeout_ms = 15000;  // 15s
            setCpuFrequencyMhz(240);
            break;
        case PROFILE_POWERSAVE:
            sleep_timeout_ms = 8000;   // 8s
            setCpuFrequencyMhz(160);
            instance.powerControl(POWER_GPS, false);
            break;
    }
}

PowerProfile power_hal_get_profile(void) {
    return current_profile;
}

bool power_hal_screen_is_off(void) {
    return screen_off;
}

void power_hal_screen_toggle(void) {
    screen_off = !screen_off;
    if (screen_off) {
        instance.setBrightness(0);
    } else {
        instance.setBrightness(PIPBOY_DEFAULT_BRIGHTNESS);
        power_hal_reset_activity();
    }
}
