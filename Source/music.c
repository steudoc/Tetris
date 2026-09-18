#include "LPC17xx.h"
#include "../timer/timer.h"
#include "music.h"

const uint16_t SinTable[30] = {
    512, 619, 721, 814, 893, 955, 998, 1020, 1020, 998, 
    955, 893, 814, 721, 619, 512, 405, 303, 210, 131, 
    69,  26,  4,   0,   4,   26,  69,  131, 210, 303
};

const uint32_t noteFreq[] = {
	1264,  // 0: E5 
	1686,  // 1: B4 
	1593,  // 2: C5 
	1419,  // 3: D5 
	1893,  // 4: A4 
  0,      // 5: null
  796,  // 6: C6 Line clear
  3787  // 7: A3 Game over
};

// Tetris theme
const uint8_t melody[] = {
	0, 1, 2, 3, 2, 1, 4, // E, B, C, D, C, B, A
	4, 2, 0, 3, 2, 1,    // A, C, E, D, C, B
	1, 2, 3, 0, 2, 4, 4  // B, C, D, E, C, A, A
};

const uint16_t duration[] = {
	400, 200, 200, 400, 200, 200, 400, 
	200, 200, 400, 200, 200, 400,      
	200, 200, 400, 400, 400, 400, 400  
};

int melodyIndex = 0;
int backgroundActive = 0;
volatile int effectActive = 0;
volatile uint16_t volume = 20;

void music_init(void) {
	LPC_PINCON->PINSEL1 &= ~(3 << 20);
	LPC_PINCON->PINSEL1 |=  (2 << 20);

	LPC_SC->PCONP |= (1 << 22); // Timer 2
	LPC_SC->PCONP |= (1 << 23); // Timer 3
	
	disable_timer(2);
	disable_timer(3);
	reset_timer(2);
	reset_timer(3);

	NVIC_EnableIRQ(TIMER2_IRQn);
	NVIC_EnableIRQ(TIMER3_IRQn);

	NVIC_SetPriority(TIMER2_IRQn, 0);
	NVIC_SetPriority(TIMER3_IRQn, 2);
}

void playNote(uint8_t idx) {
	if (idx >= 8) { 
			stopNote();
			return;
	}
	
	disable_timer(2);
	reset_timer(2);
	init_timer(2, 0, 0, 3, noteFreq[idx]); 
	enable_timer(2);
}

void stopNote(void) {
	disable_timer(2);
	LPC_DAC->DACR = 0;
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

void music_play_effect(uint8_t id) {
	disable_timer(3);
	reset_timer(3);
	
	effectActive = 1;
	
	if (id == EFFECT_LINE_CLEAR) {
			playNote(6); // C6 
			init_timer(3, 24999, 0, 3, 500); // 500ms
	} 
	else if (id == EFFECT_GAME_OVER) {
			playNote(7); // A3
			init_timer(3, 24999, 0, 3, 3000); 	// 3 s
	}
	
	NVIC_ClearPendingIRQ(TIMER3_IRQn);
	enable_timer(3);
}

void music_update_volume(uint16_t adc) {
	uint16_t new_volume;
	
	if (adc < 200) {
		volume = 0;
		return;
	}
	
	new_volume = adc >> 6;
	if (new_volume > volume + 1 || (volume > 0 && new_volume < volume-1)) {
		volume = new_volume;
	}
}