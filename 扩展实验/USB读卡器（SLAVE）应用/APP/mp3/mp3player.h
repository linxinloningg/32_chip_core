#ifndef __MP3PLAYER_H
#define __MP3PLAYER_H

#include "system.h"

extern uint8_t MP3_Volume;

int8_t MP3_Init(void);
void MP3_AudioSetting(uint8_t vol);
void MP3_BaseSetting(uint8_t amplitudeH, uint8_t freqLimitH, uint8_t amplitudeL, uint8_t freqLimitL);
void MP3_EffectSetting(uint8_t effect);
void MP3_PlaySong(uint8_t *addr);


































#endif
