/*********************************************************************************************************
**--------------File Info---------------------------------------------------------------------------------
** File name:           IRQ_RIT.c
** Last modified Date:  2014-09-25
** Last Version:        V1.00
** Descriptions:        functions to manage T0 and T1 interrupts
** Correlated files:    RIT.h
**--------------------------------------------------------------------------------------------------------
*********************************************************************************************************/
#include "LPC17xx.h"
#include "RIT.h"
#include "../led/led.h"
#include "tetris.h"
#include "adc/adc.h"

/******************************************************************************
** Function name:		RIT_IRQHandler
**
** Descriptions:		REPETITIVE INTERRUPT TIMER handler
**
** parameters:			None
** Returned value:		None
**
******************************************************************************/

volatile int down=0;
volatile int drop=0;
volatile int joystickDown = 0;

void RIT_IRQHandler (void)
{					
	static int J_up = 0;
	static int J_down = 0;
	static int J_right = 0;
	static int J_left = 0;
	
	fallCounter++;
	ADC_start_conversion();		// for the speed
	
	if((LPC_GPIO1->FIOPIN & (1<<29)) == 0){	
		/* Joytick UP pressed -> ROTATE */
		J_up++;
		switch(J_up){
			case 1:
				//rotateTetromino();
			reqFlag = REQ_ROTATE;
				break;
		
			default:
				if(J_up %8 == 0){
				  //rotateTetromino();
					reqFlag = REQ_ROTATE;
				}
			
			
				break;
		}
	}
	else{
			J_up=0;
	}
	
	if((LPC_GPIO1->FIOPIN & (1<<28)) == 0){	
		/* Joytick RIGHT pressed -> MOVE RIGHT */
		J_right++;
		switch(J_right){
			case 1:
				//moveRight();
			reqFlag = REQ_RIGHT;
				break;
		
			default:
				if(J_right %8 == 0){
				  //moveRight();
					reqFlag = REQ_RIGHT;
				}
			
			
				break;
		}
	}
	else{
			J_right=0;
	}
	
	if((LPC_GPIO1->FIOPIN & (1<<27)) == 0){	
		/* Joytick LEFT pressed -> MOVE LEFT */
		J_left++;
		switch(J_left){
			case 1:
				//moveLeft();
				reqFlag = REQ_LEFT;
				break;
		
			default:
				if(J_left %8 == 0){
				  //moveLeft();
					reqFlag = REQ_LEFT;
				}
			
			
				break;
		}
	}
	else{
			J_left=0;
	}
	
  if((LPC_GPIO1->FIOPIN & (1<<26)) == 0){	
		/* Joytick DOWN pressed -> SOFT DROP*/
		J_down++;
		switch(J_down){
			case 1:
				joystickDown = 1;
				break;
			
			default:
				if(J_down %8 == 0){
				  joystickDown = 1;
				}
				break;
		}
	}
	else{
			J_down=0;
			joystickDown = 0;
	}
	
	/* button management */
	if(down>=1){ 
		if((LPC_GPIO2->FIOPIN & (1<<11)) == 0){	/* KEY1 pressed */
			switch(down){
				case 2:				/* pay attention here: please see slides to understand value 2 */
					togglePause();
					break;
				default:
					break;
			}
			down++;
		} else {	/* button released */
			down=0;			
			NVIC_EnableIRQ(EINT1_IRQn);							 /* enable Button interrupts			*/
			LPC_PINCON->PINSEL4    |= (1 << 22);     /* External interrupt 0 pin selection */
		}
	}
/*	else{
			if(down==1)
				down++;
	
	} */
	if(drop>=1) {
		if((LPC_GPIO2->FIOPIN & (1<<12)) == 0){	/* KEY2 pressed */
			switch(drop){				
				case 2:				/* pay attention here: please see slides to understand value 2 */
					//hardDrop();
					reqFlag = REQ_HARDDROP;
					break;
				default:
					break;
			}
			drop++;
		} else {	/* button released */
			drop=0;			
			NVIC_EnableIRQ(EINT2_IRQn);							 /* enable Button interrupts			*/
			LPC_PINCON->PINSEL4    |= (1 << 24);     /* External interrupt 0 pin selection */
		}
	}
	
  LPC_RIT->RICTRL |= 0x1;	/* clear interrupt flag */
  return;
}

/******************************************************************************
**                            End Of File
******************************************************************************/
