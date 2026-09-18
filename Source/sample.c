/*----------------------------------------------------------------------------
 * Name:    sample.c
 * Purpose: to control led through debounced buttons and Joystick
 *        	- key1 switches on the led at the left of the current led on, 
 *					- it implements a circular led effect,
 * 					- joystick UP function returns to initial configuration (led11 on) .
 * Note(s): this version supports the LANDTIGER Emulator
 * Author: 	Paolo BERNARDI - PoliTO - last modified 15/12/2020
 *----------------------------------------------------------------------------
 *
 * This software is supplied "AS IS" without warranties of any kind.
 *
 * Copyright (c) 2017 Politecnico di Torino. All rights reserved.
 *----------------------------------------------------------------------------*/
                  
#include <stdio.h>
#include "LPC17xx.h"                    /* LPC17xx definitions                */
#include "led/led.h"
#include "button_EXINT/button.h"
#include "timer/timer.h"
#include "RIT/RIT.h"
#include "joystick/joystick.h"
#include "GLCD/GLCD.h"
#include "tetris.h"
#include "adc/adc.h"
#include "music.h"

#ifdef SIMULATOR
extern uint8_t ScaleFlag; // <- ScaleFlag needs to visible in order for the emulator to find the symbol (can be placed also inside system_LPC17xx.h but since it is RO, it needs more work)
#endif
/*----------------------------------------------------------------------------
  Main Program
 *----------------------------------------------------------------------------*/
int main (void) {
 
	SystemInit();  												/* System Initialization (i.e., PLL)  */
	LCD_Initialization();
	
  LED_init();                           /* LED Initialization                 */
  BUTTON_init();												/* BUTTON Initialization              */
	joystick_init();											/* Joystick Initialization            */
	
	music_init();
	
	//init_RIT(0x004C4B40);									/* RIT Initialization 50 msec       	*/
	init_RIT(0x00186A00); // 20 ms
	NVIC_SetPriority(RIT_IRQn, 1);
	enable_RIT();													/* RIT enabled												*/
	init_timer(0, 0, 0, 3, 0xFFFFFFFF);		// Timer 0 initialization
	enable_timer(0);											// Timer 0 enabled
	ADC_init();
	
	initGame();
	
	//LPC_SC->PCON |= 0x1;									/* power-down	mode										*/
	//LPC_SC->PCON &= ~(0x2);	

	//LPC_PINCON->PINSEL1 &= ~(3 << 20); 
	//LPC_PINCON->PINSEL1 |=  (1 << 20);

		
  while (1) {                           /* Loop forever                       */	
		updateGame();		// game's logic
		__ASM("wfi");		// wait for interrupt
  }

}
