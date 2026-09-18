#ifndef MUSIC_H
#define MUSIC_H

#include <stdint.h>

void music_init(void);
void music_update_volume(uint16_t adc);
void music_start_background(void);
void music_stop_background(void);
void music_play_effect(uint8_t id);

void playNote(uint8_t idx);
void stopNote(void);

extern int melodyIndex;
extern int backgroundActive;

#define MELODY_SIZE 19
#define EFFECT_LINE_CLEAR 0
#define EFFECT_GAME_OVER  1

extern volatile int effectActive;
extern const uint8_t melody[];
extern const uint16_t duration[];
extern const uint16_t SinTable[30];
extern volatile uint16_t volume;

#endif
