#pragma once
#include <LilyGoLib.h>

#if defined(USING_ST25R3916)
#include "nfc_hal.h"

// Start T2T card emulation with given UID and NDEF text
bool nfc_emu_start(const uint8_t *uid, uint8_t uid_len, const char *ndef_text);

// Must be called frequently during emulation - handles reader commands
// Returns true if a reader interacted with us
bool nfc_emu_loop(void);

// Stop emulation
void nfc_emu_stop(void);

bool nfc_emu_is_active(void);

#endif
