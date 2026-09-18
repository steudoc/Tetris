/*********************************************************************************************************
**--------------File Info---------------------------------------------------------------------------------
** File name:           IRQ_timer.c
** Last modified Date:  2014-09-25
** Last Version:        V1.00
** Descriptions:        functions to manage T0 and T1 interrupts
** Correlated files:    timer.h
**--------------------------------------------------------------------------------------------------------
*********************************************************************************************************/
#include "LPC17xx.h"
#include "timer.h"
#include "../led/led.h"
#include "tetris.h"
#include "music.h"

extern const uint16_t SinTable[30];
extern volatile uint16_t volume;
extern int backgroundActive;
extern int melodyIndex;
extern const uint8_t melody[];
extern const uint16_t duration[];

extern volatile int effectActive;

/******************************************************************************
** Function name:		Timer0_IRQHandler
**
** Descriptions:		Timer/Counter 0 interrupt handler
**
** parameters:			None
** Returned value:		None
**
******************************************************************************/
extern unsigned char led_value;					/* defined in funct_led								*/
void TIMER0_IRQHandler (void)
{
	
	/* Match register 0 interrupt service routine */
	if (LPC_TIM0->IR & 01)
	{
		disable_timer(0);
		reset_timer(0);
		enable_timer(0);
		
		LPC_TIM0->IR = 1;			/* clear interrupt flag */
	}
		/* Match register 1 interrupt service routine */
	  /* it should be possible to access to both interrupt requests in the same procedure*/
	else if(LPC_TIM0->IR & 02)
  {
		

		LPC_TIM0->IR =  2 ;			/* clear interrupt flag */	
	}
	/* Match register 2 interrupt service routine */
  /* it should be possible to access to both interrupt requests in the same procedure*/
	else if(LPC_TIM0->IR & 4)
  {
		
		
		LPC_TIM0->IR =  4 ;			/* clear interrupt flag */	
	}
		/* Match register 3 interrupt service routine */
  	/* it should be possible to access to both interrupt requests in the same procedure*/
	else if(LPC_TIM0->IR & 8)
  {
	 
		LPC_TIM0->IR =  8 ;			/* clear interrupt flag */	
	}
  return;
}


/******************************************************************************
** Function name:		Timer1_IRQHandler
**
** Descriptions:		Timer/Counter 1 interrupt handler
**
** parameters:			None
** Returned value:		None
**
******************************************************************************/
void TIMER1_IRQHandler (void)
{
	slowDownActive = 0;
	
	disable_timer(1);
	
  LPC_TIM1->IR = 1;			/* clear interrupt flag */
  return;
}

/******************************************************************************
** Function name:		Timer2_IRQHandler
**
** Descriptions:		Timer/Counter 2 interrupt handler
**
** parameters:			None
** Returned value:		None
**
******************************************************************************/
void TIMER2_IRQHandler (void)
{
	static int ticks = 0;
	uint32_t sample;
	
	if (volume == 0) {
		LPC_DAC->DACR = 0;
		
		ticks++;
		if (ticks>=30) {
			ticks = 0;
		}
		
		LPC_TIM2->IR = 1;
		return;
	}

	sample = SinTable[ticks];    
	sample = (sample * volume) >> 6; // Shift veloce
	
	if (sample > 1023) {
		sample = 1023;
	}
	LPC_DAC->DACR = (sample << 6);

	ticks++;
	if (ticks >= 30) {
		ticks = 0;
	}

	LPC_TIM2->IR = 1; 
	return;
}


void TIMER3_IRQHandler (void) {
	LPC_TIM3->IR = 1; // Clear flag
 
	if (effectActive) {
			effectActive = 0; // effect finished
			
			if (backgroundActive) {
					playNote(melody[melodyIndex]);
					
					// restart timer for background music
					disable_timer(3);
					reset_timer(3);
					init_timer(3, 24999, 0, 3, duration[melodyIndex]);
					NVIC_ClearPendingIRQ(TIMER3_IRQn);
					enable_timer(3);
			} else {
					stopNote();
			}
			return;
	}

	if (!backgroundActive) {
			stopNote();
			return;
	}

	melodyIndex++;
	if (melodyIndex >= MELODY_SIZE)
			melodyIndex = 0;

	playNote(melody[melodyIndex]);

	disable_timer(3);
	reset_timer(3);
	init_timer(3, 24999, 0, 3, duration[melodyIndex]);
	enable_timer(3);
	return;
}


/******************************************************************************
**                            End Of File
******************************************************************************/
