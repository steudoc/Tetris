#include "GLCD/GLCD.h"
#define TETRIS_H

// game field
#define FIELD_ROWS 20
#define FIELD_COLS 10
#define FIELD_OFFSET_X 0
#define FIELD_OFFSET_Y 0
#define BLOCK_SIZE 16
#define FALL_NORMAL 50
#define FALL_FAST 25

#define COLOR_POWERUP_BG	Grey
#define COLOR_POWERUP_FG	Red

extern int fallCounter;
extern int fallSpeed;
extern int currentSpeed;

typedef enum {
	I_BLOCK,
	O_BLOCK,
	T_BLOCK,
	J_BLOCK,
	L_BLOCK,
	S_BLOCK,
	Z_BLOCK
} TetrominoType;

static uint16_t tetrominoColors[] = {
	Cyan,		 // I
	Yellow,  // O
	Magenta, // T
	Blue,	 	 // J
	Orange,	 // L
	Green, 	 // S
	Red	 		 // Z
};

typedef struct {
	int x, y;
} Block;

typedef struct {
	Block blocks[4];
	TetrominoType type;
} Tetromino;

typedef enum {
	REQ_NONE = 0,
	REQ_LEFT,
	REQ_RIGHT,
	REQ_ROTATE,
	REQ_HARDDROP
} RequestType;

extern volatile RequestType reqFlag;
extern int slowDownActive;

void initGame(void);
void spawnTetromino(void);
void updateGame(void);
void drawScore(void);
void drawField(void);
int checkLines(void);
void rotateTetromino(void);
int isPaused(void);
void togglePause(void);
void moveRight(void);
void moveLeft(void);
void softDrop(void);
void hardDrop(void);
void drawFieldBorder(void);
void addScore(int lines);
void restartGame(void);
int isGameOver(void);
void drawCell(int col, int row);
void drawFieldStatic(void);
void eraseCurrentPiece(int px, int py);
void drawCurrentPiece(void);
void drawLockedBlock(int X, int Y, int type);
void spawnPowerup(void);
void activatePowerup(int type);
void applyMalus(void);



