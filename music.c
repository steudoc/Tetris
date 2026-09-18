#include "LPC17xx.h"
#include "music.h"
#include "../timer/timer.h"

#define BUZZER_PIN   (1<<0)

static const uint32_t noteFreq[] = { // note frequencies
    2120, // C4
    1890, // D4
    1684, // E4
    1592, // F4
    1417, // G4
    1263, // A4
    1125, // B4
    1062  // C5
};

const uint8_t melody[] = {2,2,3,4,4,3,2,1,0,0,1,2,2,1,1}; // background music
const uint16_t duration[] = {200,200,200,200,200,200,200,200,400,200,200,200,200,200,400};
int melodyIndex = 0;
int backgroundActive = 0;

static uint16_t volume = 4095;   // max volume

void music_update_volume(uint16_t adcValue) {
	volume = adcValue; // 0..4095
}

void music_init(void) {
	LPC_PINCON->PINSEL4 &= ~(3<<0);
	LPC_GPIO2->FIODIR |= BUZZER_PIN;
}

static void playNote(uint8_t noteIndex) {
	disable_timer(2);
	reset_timer(2);

	// Timer2 with MR0 = note frequency
	init_timer(2, 0, 0, 3, noteFreq[noteIndex]);
	enable_timer(2);
}

static void stopNote(void) {
	disable_timer(2);
	LPC_GPIO2->FIOCLR = BUZZER_PIN;
}

void music_play_sound(uint8_t effectID) {
    switch(effectID) {
        case 0: playNote(6); break; // rotate
        case 1: playNote(4); break; // move
        case 2: playNote(0); break; // hard drop
        case 3: playNote(7); break; // line clear
        case 4: playNote(0); break; // game over
    }

    // Timer3 = duration
    disable_timer(3);
    reset_timer(3);
    init_timer(3, 24999, 0, 3, 80); // 80ms
    enable_timer(3);
}

void music_start_background(void) {
    melodyIndex = 0;
    backgroundActive = 1;

    playNote(melody[melodyIndex]);

    disable_timer(3);
    reset_timer(3);
    init_timer(3, 24999, 0, 3, duration[melodyIndex]);
    enable_timer(3);
}

void music_stop_background(void) {
    backgroundActive = 0;
    stopNote();
    disable_timer(3);
}






