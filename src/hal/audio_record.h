#pragma once
#include <Arduino.h>

extern volatile bool audio_rec_active;

bool audio_rec_start(const char* filename);
void audio_rec_stop(void);
bool audio_rec_is_recording(void);
const char* audio_rec_get_filename(void);
