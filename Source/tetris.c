#include "tetris.h"
#include "joystick/joystick.h"
#include "button_EXINT/button.h"
#include "timer/timer.h"
#include "adc/adc.h"
#include "RIT/RIT.h"
#include <stdlib.h>
#include <stdio.h>
#include "music.h"

static int field[FIELD_ROWS][FIELD_COLS];
static int powerupField[FIELD_ROWS][FIELD_COLS]; //0 = no powerup, 1 = ClearHalf, 2 = SlowDown
static Tetromino tetrominoes[7] = {
    // I
    {{ {0,-1}, {0,0}, {0,1}, {0,2} }},
    // O
    {{ {0,0}, {0,1}, {1,0}, {1,1} }},
    // T
    {{ {0,-1}, {0,0}, {0,1}, {1,0} }},
    // J
    {{ {0,-1}, {0,0}, {0,1}, {1,-1} }},
    // L
    {{ {0,-1}, {0,0}, {0,1}, {1,1} }},
    // S
    {{ {0,0}, {0,1}, {1,-1}, {1,0} }},
    // Z
    {{ {0,-1}, {0,0}, {1,0}, {1,1} }}
};

volatile RequestType reqFlag = REQ_NONE;

static Tetromino currentPiece;
static int x, y;
static int paused = 1;
static int score = 0;
static int bestScore = 0;
static int linesCleared = 0;
static int gameOver = 0;
static int restartScheduled = 0;
static int scoreX = 170;
static int seedInitialized = 0;
static int firstPiece = 1;

int fallCounter = 0;
int fallSpeed = FALL_NORMAL;

extern unsigned short AD_current;
uint16_t adc;
int baseSpeed, currentSpeed = 1;

static int linesSincePowerup = 0;
static int linesSinceMalus = 0;
int slowDownActive = 0;

void initGame(void) {
	//srand(LPC_TIM0->TC^LPC_RIT->RICOUNTER);
	music_stop_background();
	
	for (int i = 0; i < FIELD_ROWS; i++) {
		for (int j = 0; j < FIELD_COLS; j++) {
			field[i][j] = 0;
		}
	}
	score = 0;
	linesCleared = 0;
	paused = 1;
	fallCounter = 0;
	gameOver = 0;
	firstPiece = 1;
	slowDownActive = 0;
	
	LCD_Clear(Black);
	drawFieldStatic();
	drawScore();
	//spawnTetromino();
	//drawCurrentPiece();
}

void spawnTetromino(void) {
	int X, Y, i = rand() % 7;
	
	currentPiece = tetrominoes[i];
	currentPiece.type = i;
	x = FIELD_COLS/2 - 1;
	y = 0;
	
	// check overlap
	for (i = 0; i < 4; i++) {
		X = x + currentPiece.blocks[i].x;
		Y = y + currentPiece.blocks[i].y;
		
		if (Y >= 0 && field[Y][X] != 0) {
			gameOver = 1;
			return;
		}
	}
}

void rotateTetromino(void) {
	int X, Y, oldX, oldY;
	Tetromino backup = currentPiece;
	
	if (gameOver || paused) {
		return;
	}
	
	// try local rotation
	for (int i = 0; i < 4; i++) {
		oldX = backup.blocks[i].x;
		oldY = backup.blocks[i].y;
		backup.blocks[i].x = -oldY;
		backup.blocks[i].y = oldX;
	}

	// check validity of the rotation
	for (int i = 0; i < 4; i++) {
		X = x + backup.blocks[i].x;
		Y = y + backup.blocks[i].y;

		if (X < 0 || X >= FIELD_COLS || Y < 0 || Y >= FIELD_ROWS) {
				return; // invalid rotation
		}
		if (field[Y][X] != 0) {
				return; // collide with locked blocks
		}
	}

	// valid rotation
	eraseCurrentPiece(x, y);
	currentPiece = backup;
	drawCurrentPiece();
}

static int canMove(int dx, int dy) {
	int X, Y;
	
	for (int i = 0; i < 4; i++) {
		X = x + currentPiece.blocks[i].x + dx;
		Y = y + currentPiece.blocks[i].y + dy;
		
		if (X < 0 || X >= FIELD_COLS || Y < 0 || Y >= FIELD_ROWS){
			return 0;
		}
		
		if (field[Y][X] != 0){
			return 0;
		}
	}
	return 1;
}

static void lockPiece(void) {
	int X, Y;

	// game over check
	for (int i = 0; i < 4; i++) {
		X = x + currentPiece.blocks[i].x;
		Y = y + currentPiece.blocks[i].y;
		
		// if piece end out of field -> game over
		if (Y < 0) {
			gameOver = 1;
			music_stop_background();
			music_play_effect(EFFECT_GAME_OVER);
			return;
		}

		// if first cell already full -> game over
		if (Y >= 0 && Y < FIELD_ROWS && field[Y][X] != 0) {
			gameOver = 1;
			music_stop_background();
			music_play_effect(EFFECT_GAME_OVER);
			return;
		}
	}

	// lock piece
	for (int i = 0; i < 4; i++) {
		X = x + currentPiece.blocks[i].x;
		Y = y + currentPiece.blocks[i].y;

		if (Y >= 0 && Y < FIELD_ROWS && X >= 0 && X < FIELD_COLS) {
			field[Y][X] = currentPiece.type + 1;
			drawCell(X,Y);
		}
	}
}

void updateGame(void) {
	int oldX, oldY, adcPollCounter = 0;
	
	if (gameOver) {	
		restartGame();
		return;
	}
	
	if (paused) {
		return;
	}
	
	if (firstPiece) {
		spawnTetromino();        
		drawCurrentPiece();      
		firstPiece = 0; 
	}
	
	switch (reqFlag) {
		case REQ_ROTATE:
			rotateTetromino();
			break;
		case REQ_RIGHT:
			moveRight();
			break;
		case REQ_LEFT:
			moveLeft();
			break;
		case REQ_HARDDROP:
			hardDrop();
			break;
		default:
			break;
	}
	reqFlag = REQ_NONE;
	
	// manage music volume
	adcPollCounter++;
	if (adcPollCounter > 2000) {
		ADC_start_conversion();
		music_update_volume(AD_current);
		
		adcPollCounter = 0;
	}
	
	// manage velocity
	if (slowDownActive) {
		baseSpeed = 1;
	} else {
		adc = AD_current;
		baseSpeed = 1 + (adc*4) / 4095;  // max val potentiometer
	}
	currentSpeed = baseSpeed;
	
	if (joystickDown) {		// SOFT DROP
		currentSpeed = baseSpeed * 2;
	}
	
	fallSpeed = FALL_NORMAL / currentSpeed;
	if (fallSpeed < 1) {
		fallSpeed = 1;
	}		
	
	// normal drop
	if (fallCounter >= fallSpeed) {
		fallCounter = 0;
		oldX = x;
		oldY = y;
		
		eraseCurrentPiece(oldX,oldY);
		
		if (canMove(0,1)) {
			y++;
			drawCurrentPiece();
		} else {
			lockPiece();
			
			if (gameOver) {
				return;
			}
			addScore(checkLines());
			drawScore();
			spawnTetromino();
			
			if (gameOver) {
				return;
			}
			
			drawCurrentPiece();
		}
		//drawField();
	}	
}

int checkLines(void) {
	int cleared = 0, full, col;
	int write_row = FIELD_ROWS - 1;
	int triggerClearHalf = 0, triggerSlowDown = 0;
	
	for (int read_row = FIELD_ROWS - 1; read_row >= 0; read_row--) {
		full = 1;
		
		for (col = 0; col < FIELD_COLS; col++) {
			if (field[read_row][col] == 0){
				full = 0;
				break;
			}
		}
		
		if (full) { 	// if full, skip row <- write_row not incremented
			// check poweup
			for (col = 0; col < FIELD_COLS; col++) {
				if (powerupField[read_row][col] == 1) {
					triggerClearHalf = 1;
				} else if (powerupField[read_row][col] == 2) {
					triggerSlowDown = 1;
				}
			}
			cleared++;
			music_play_effect(EFFECT_LINE_CLEAR);
		} else {
			// if not full, copied in position write_row
			if (write_row != read_row) { // -> means rows canceled below
				for (col = 0; col < FIELD_COLS; col++) {
					field[write_row][col] = field[read_row][col];
					powerupField[write_row][col] = powerupField[read_row][col];
				}
			}
			write_row--;
		}
	}
	
	if (triggerSlowDown) {
		activatePowerup(2);
	}
	if (triggerClearHalf) {
		activatePowerup(1);
		music_play_effect(EFFECT_LINE_CLEAR);
		return 0; // score already managed in activatePowerup
	}

	if (cleared == 0) {
		return 0;
	}
	
	while (write_row >= 0) {  // fill with 0 rows from position write_row to 0
		for (col = 0; col < FIELD_COLS; col++) {
			field[write_row][col] = 0;
			powerupField[write_row][col] = 0;
		}
		write_row--;
	}
	
	linesCleared += cleared;
	drawFieldStatic();
	
	return cleared;
}

void activatePowerup (int type) {
	if (type == 1) {				// CLEAR HALF
		int firstOccupiedRow = FIELD_ROWS, linesToRemove, row, col, srcRow;
		
		for (row = 0; row < FIELD_ROWS; row++) {
			for (col = 0; col< FIELD_COLS; col++){
				if (field[row][col] != 0) {
					firstOccupiedRow = row;
					break;
				}
			}
			if (firstOccupiedRow != FIELD_ROWS) {
				break;
			}
		}
		
		if (firstOccupiedRow == FIELD_ROWS) {
			return;
		}
		
		linesToRemove = (FIELD_ROWS - firstOccupiedRow) / 2;
		if (linesToRemove == 0) {
			return;
		}
		
		// shift down rows
		for (row = FIELD_ROWS-1; row >= linesToRemove; row--) {
			for (col = 0; col<FIELD_COLS; col++) {
				srcRow = row - linesToRemove;
				if (srcRow >= 0) {
					field[row][col] = field[srcRow][col];
					powerupField[row][col] = powerupField[srcRow][col];
				} else {
					field[row][col] = 0;
					powerupField[row][col] = 0;
				}
			}
		}
		
		// clear upper-half field
		for (row = 0; row < linesToRemove; row++) {
			for (col = 0; col < FIELD_COLS; col++) {
				field[row][col] = 0;
				powerupField[row][col] = 0;
			}
		}
		
		linesCleared += linesToRemove;
		linesSincePowerup += linesToRemove;
		linesSinceMalus += linesToRemove;
		
		// add points in goups of 4
		while (linesToRemove >= 4) {
			score += 600;
			linesToRemove = -4;
		}
		if (linesToRemove > 0) {
			score += linesToRemove * 100;
		}
		
		while (linesSincePowerup >= 5) {
			linesSincePowerup -= 5;
			spawnPowerup();
		}
		while (linesSinceMalus >= 10) {
			linesSinceMalus -= 10;
			applyMalus();
		}
				
		drawFieldStatic();
		
	} else if (type == 2) {	// SLOW DOWN
		if (!slowDownActive) {
			slowDownActive = 1;
			
			reset_timer(1);
			init_timer(1, 24999, 0, 3, 15000);
			enable_timer(1);
		}
	}
}

void drawScore(void) {	
	char buf[32];
	
	GUI_Text(scoreX, 20, (uint8_t*)"SCORE", White, Black);
	sprintf(buf, "%d", score);
	GUI_Text(scoreX, 35, (uint8_t*)buf, White, Black);
	
	GUI_Text(scoreX, 60, (uint8_t*)"LINES", White, Black);
	sprintf(buf, "%d", linesCleared);
	GUI_Text(scoreX, 75, (uint8_t*)buf, White, Black);
	
	GUI_Text(scoreX, 100, (uint8_t*)"BEST", White, Black);
	sprintf(buf, "%d", bestScore);
	GUI_Text(scoreX, 115, (uint8_t*)buf, White, Black);
}

void drawField(void) {
	//LCD_Clear(Black);
	int X, Y;
	uint16_t color;
	
	drawFieldBorder();
	
	for (int i = 0; i < FIELD_ROWS; i++) {
		for (int j = 0; j < FIELD_COLS; j++) {
			if (field[i][j] == 0) {
				LCD_DrawRect(FIELD_OFFSET_X + j*BLOCK_SIZE, FIELD_OFFSET_Y + i*BLOCK_SIZE, BLOCK_SIZE, BLOCK_SIZE, Black);
			} else {
				color = tetrominoColors[field[i][j] - 1];
				LCD_DrawRect(FIELD_OFFSET_X + j*BLOCK_SIZE, FIELD_OFFSET_Y + i*BLOCK_SIZE, BLOCK_SIZE, BLOCK_SIZE, color);
			}
		}
	}
	
	// current piece
	for (int i = 0; i < 4; i++) {
			X = x + currentPiece.blocks[i].x;
			Y = y + currentPiece.blocks[i].y;

			if (Y >= 0) {
					color = tetrominoColors[currentPiece.type];
					LCD_DrawRect(FIELD_OFFSET_X + X*BLOCK_SIZE, FIELD_OFFSET_Y + Y*BLOCK_SIZE, BLOCK_SIZE, BLOCK_SIZE, color);
			}
	}
}

int isPaused(void) {
	return paused;
}

void togglePause(void) {
	if (!seedInitialized) {
		uint32_t seed = LPC_TIM0->TC & 0xFF;
		srand(seed);
		seedInitialized = 1;			
	}
	
	paused = !paused;
	if (paused) {
		music_stop_background();
	} else {
		music_start_background();
	}
}


void moveLeft(void) {
	if (gameOver || paused) {
		return;
	}
	
	if (canMove(-1,0)){
		eraseCurrentPiece(x,y);
		x--;
		drawCurrentPiece();
	}
}

void moveRight(void) {
	if (gameOver || paused) {
		return;
	}
	
	if (canMove(1,0)) {
		eraseCurrentPiece(x,y);
		x++;
		drawCurrentPiece();
	}
}

int isGameOver(void) {
	return gameOver;
}

void hardDrop(void) {
	if (gameOver || paused) {
		return;
	}
	
	eraseCurrentPiece(x,y);
	
	while (canMove(0,1)) {
		y++;
	}
	drawCurrentPiece();
	lockPiece();
	
	if (gameOver) {
		return;
	}
	fallCounter = 0;
	addScore(checkLines());
	drawScore();
	spawnTetromino();
	
	if (!gameOver) {
		drawCurrentPiece();
	}
}

void drawFieldBorder(void) {
	int x0 = FIELD_OFFSET_X + FIELD_COLS * BLOCK_SIZE + 1;
	int y0 = FIELD_OFFSET_Y;
	int y1 = FIELD_OFFSET_Y + FIELD_ROWS * BLOCK_SIZE;
	
	LCD_DrawLine(x0, y0, x0, y1, White);	// delimiter
	
}

void addScore(int lines) {
	// 10 pt for positioning
	score += 10;
	
	if (lines == 4) {
		score += 600;		// tetris
	} else {
		score += (lines*100);
	}
	
	linesSincePowerup += lines;
	linesSinceMalus += lines;
	
	if (linesSincePowerup >= 5) {
		linesSincePowerup -=5;
		spawnPowerup();
	}
	
	if (linesSinceMalus >= 10) {
		linesSinceMalus -= 10;
		applyMalus();
	}
}

void spawnPowerup(void) {
	int candidates[FIELD_ROWS * FIELD_COLS][2];
	int count = 0, row, col, i, powerUp;
	
	// find all non-empty cells
	for (row = 0; row<FIELD_ROWS; row++) {
		for (col = 0; col<FIELD_COLS; col++) {
			if (field[row][col] != 0) {
				candidates[count][0] = row;
				candidates[count][1] = col;
				count++;
			}
		}
	}
	
	if (count == 0) {
		return;
	}
	
	i = rand() % count;
	row = candidates[i][0];
	col = candidates[i][1];
	
	powerUp = (rand() % 2) + 1;	// 1 = ClearHalf, 2 = SlowDown
	powerupField[row][col] = powerUp;
	drawCell(col, row);
}

void restartGame(void) {
	GUI_Text(scoreX, 150, (uint8_t*)"GAME", Red, Black);
	GUI_Text(scoreX, 165, (uint8_t*)"OVER", Red, Black);

	if (score > bestScore) {
		bestScore = score;
	}
	
	initGame();
}

void drawCell(int col, int row)
{
	uint16_t color;
	int px, py;
	px = FIELD_OFFSET_X + col * BLOCK_SIZE;
	py = FIELD_OFFSET_Y + row * BLOCK_SIZE;
	
	if (powerupField[row][col] == 1) {	// powerup CLEAR-HALF
		LCD_DrawRect(px, py, BLOCK_SIZE, BLOCK_SIZE, COLOR_POWERUP_BG);
		//LCD_DrawRect(px+3, py+3, BLOCK_SIZE - 6, BLOCK_SIZE - 6, COLOR_POWERUP_FG);
		//GUI_Text(px + 4, py, (uint8_t *)"C", COLOR_POWERUP_FG, COLOR_POWERUP_BG);
		LCD_DrawLine(px+4, py+3, px+11, py+3, COLOR_POWERUP_FG);
		LCD_DrawLine(px+4, py+3, px+4, py+12, COLOR_POWERUP_FG);
		LCD_DrawLine(px+4, py+12, px+11, py+12, COLOR_POWERUP_FG);
		return;		
	} else if (powerupField[row][col] == 2) {		// powerup SLOW-DOWN
		LCD_DrawRect(px, py, BLOCK_SIZE, BLOCK_SIZE, COLOR_POWERUP_BG);
		//LCD_DrawLine(px+3, py+3, px+BLOCK_SIZE-4, py+BLOCK_SIZE-4, COLOR_POWERUP_FG);
		//LCD_DrawLine(px+BLOCK_SIZE-4,py+3, px+3, py+BLOCK_SIZE-4, COLOR_POWERUP_FG);
		//GUI_Text(px + 4, py, (uint8_t *)"S", COLOR_POWERUP_FG, COLOR_POWERUP_BG);
		LCD_DrawLine(px+4, py+3, px+11, py+3, COLOR_POWERUP_FG);   // top
		LCD_DrawLine(px+4, py+3, px+4, py+7, COLOR_POWERUP_FG);    // sx 
		LCD_DrawLine(px+4, py+7, px+11, py+7, COLOR_POWERUP_FG);   // center
		LCD_DrawLine(px+11, py+7, px+11, py+12, COLOR_POWERUP_FG); // dx 
		LCD_DrawLine(px+4, py+12, px+11, py+12, COLOR_POWERUP_FG); // bottom
		return;  // normal cell
	} else if (field[row][col] == 0) {
		color = Black;
	} else {
		color = tetrominoColors[field[row][col] - 1];
	}

	LCD_DrawRect(px, py, BLOCK_SIZE, BLOCK_SIZE, color);
}

void drawFieldStatic(void) {
	drawFieldBorder();
	
	for (int i = 0; i < FIELD_ROWS; i++) {
		for (int j = 0; j < FIELD_COLS; j++) {
				drawCell(j, i);
		}
	}
}

void eraseCurrentPiece(int px, int py) {
	int X, Y;
	
	for (int i = 0; i < 4; i++) {
		X = px + currentPiece.blocks[i].x;
		Y = py + currentPiece.blocks[i].y;

		if (Y >= 0 && Y < FIELD_ROWS && X >= 0 && X < FIELD_COLS) {
			drawCell(X, Y);
		}
	}
}

void drawCurrentPiece(void) {
	int X, Y;
	uint16_t color = tetrominoColors[currentPiece.type];

	for (int i = 0; i < 4; i++) {
		X = x + currentPiece.blocks[i].x;
		Y = y + currentPiece.blocks[i].y;

		if (Y >= 0 && Y < FIELD_ROWS && X >= 0 && X < FIELD_COLS) {
			LCD_DrawRect(FIELD_OFFSET_X + X * BLOCK_SIZE,
				FIELD_OFFSET_Y + Y * BLOCK_SIZE,
				BLOCK_SIZE, BLOCK_SIZE, color);
		}
	}
}

void drawLockedBlock(int X, int Y, int type) {
    LCD_DrawRect(
        FIELD_OFFSET_X + X * BLOCK_SIZE,
        FIELD_OFFSET_Y + Y * BLOCK_SIZE,
        BLOCK_SIZE,
        BLOCK_SIZE,
        tetrominoColors[type]
    );
}

void applyMalus(void) {
	int row, col, bottomRow, blocksToPlace = 7, pos;
	
	for (col = 0; col<FIELD_COLS; col++) {
		if (field[0][col] != 0) {
			gameOver = 1;
			break;
		}
	}
	
	// shift rows up
	for (row = 0; row < FIELD_ROWS; row++) {
		for (col = 0; col < FIELD_COLS; col++) {
			field[row][col] = field[row+1][col];
			powerupField[row][col] = powerupField[row+1][col];
		}
	}
	
	bottomRow = FIELD_ROWS -1; //malus row
	for (col = 0; col <FIELD_COLS; col++) {
		field[bottomRow][col] = 0;
		powerupField[bottomRow][col] = 0;
	}
	
	while (blocksToPlace > 0) {
		pos = rand() % FIELD_COLS;
		if (field[bottomRow][pos] == 0) {
			field[bottomRow][pos] = (rand() % 7) +1;
			blocksToPlace--;
		}
	}
	
	drawFieldStatic();
	
	if (gameOver) {
		music_stop_background();
		music_play_effect(EFFECT_GAME_OVER);
		restartGame();
	}
}






