# Tetris on LandTiger LPC1768

![Embedded Tetris Banner](./cover.jpg)

## Description
Developed for the Computer Architectures course, this project features a bare-metal implementation of the classic Tetris game for the NXP LPC1768 microcontroller on the LandTiger development board. It leverages low-level programming of the board's hardware peripherals—including the LCD display, hardware timers, Analog-to-Digital Converters (ADC), and speaker—to deliver a responsive gaming experience enhanced with custom mechanics.

## Core Competencies & Gameplay Features

* **Game Engine & Rendering:** Implementation of classic Tetris rules on a 20x10 grid. Includes full management of the 7 standard tetrominoes (I, O, T, J, L, S, Z), boundary collision detection, horizontal line clearing, and "topping out" (game over) conditions.
* **Input & Interrupt Handling:** Joystick integration for horizontal movement and clockwise piece rotation. Physical buttons are mapped to system states (KEY1 for Start/Pause) and instant Hard Drops (KEY2). The game loop and rendering are driven by the Repetitive Interrupt Timer (RIT) configured to trigger every 20 milliseconds.
* **Dynamic Speed via ADC:** The base block falling speed scales from 1 to 5 squares per second by reading the board's potentiometer values via the ADC. The Soft Drop mechanic (holding the joystick down) dynamically doubles the current falling speed.
* **Pseudo-Random Generation (RNG):** To guarantee piece unpredictability, the generation algorithm's seed is extracted from Timer 0, which runs continuously in the background until the user's first interaction (pressing KEY1).
* **Powerups & Dynamic Obstacles:** Gameplay is extended with powerups spawning every 5 cleared lines, which can either instantly clear the bottom half of the screen or force a slowdown to 1 block/sec for 15 seconds. To increase difficulty, a "Malus" effect triggers every 10 cleared lines, generating an incomplete row at the bottom and pushing the entire grid upwards.
* **Interactive Audio:** Configuration of the DAC and integrated speaker to play background music and reactive sound effects in real-time.
* **Scoring System:** A tiered scoring system based on the number of simultaneously cleared lines (awarding up to 600 points for a 4-line "Tetris"), alongside persistent High Score tracking across game sessions.

## Technologies & Hardware

* **Development Board:** LandTiger Board (NXP LPC1768 / ARM Cortex-M3 Architecture)
* **Language:** C (Bare-metal embedded programming)
* **IDE:** Keil µVision (utilized in SW_Debug emulator mode and for physical board flashing)
* **Peripherals Used:** LCD, GPIO (Joystick, Buttons), ADC (Potentiometer), DAC/Speaker, NVIC, Hardware Timers, RIT
