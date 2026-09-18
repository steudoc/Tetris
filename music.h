#ifndef MUSIC_H
#define MUSIC_H

#include <stdint.h>

extern 

void music_init(void);
void music_play_sound(uint8_t effectID);
void music_start_background(void);
void music_stop_background(void);
void music_update_value(uint16_t adcValue);

#endif
