#include <stdio.h>
#include <stdlib.h>

#include "logsys.h"
#include "input.h"
#include "graphics.h"

// Size of the stage
#define STAGE_W 10
#define STAGE_H 20
// Number of lines to clear before going to the next level
#define LINES_PER_LEVEL 20
// "SPEED" is actually number of frames here
// Initial speed is the "gravity" for level 1
#define INITIAL_SPEED 45
// Gravity for soft drop when player holds the down button
#define DROP_SPEED 5
// Size for each individual block, and also effects a number of other things
#define BLOCK_SIZE 16
// Minimum time a between a piece touching the bottom and locking
#define LOCK_DELAY 30
// For delayed auto shift, wait SHIFT_DELAY frames first,
// then wait SHIFT_SPEED while left/right continues to be held
#define SHIFT_DELAY 20
#define SHIFT_SPEED 4

// Score amounts rewarded for various actions
#define SCORE_SINGLE 100
#define SCORE_DOUBLE 300
#define SCORE_TRIPLE 500
#define SCORE_TETRIS 800
#define SCORE_EZ_TSPIN 100
#define SCORE_EZ_TSPIN_SINGLE 200
#define SCORE_TSPIN 400
#define SCORE_TSPIN_SINGLE 800
#define SCORE_TSPIN_DOUBLE 1200
#define SCORE_SOFT_DROP 1
#define SCORE_HARD_DROP 2

// Game mode, like using screens except a single variable switch instead
#define MODE_TITLE 0
#define MODE_OPTIONS 1
#define MODE_STAGE 2
#define MODE_GAMEOVER 3

// Locations and sizes that depend on the chosen block size
#define STAGE_X 6 * BLOCK_SIZE
#define STAGE_Y 2 * BLOCK_SIZE
#define SCREEN_W 22 * BLOCK_SIZE
#define SCREEN_H 24 * BLOCK_SIZE
#define QUEUE_X 17 * BLOCK_SIZE
#define QUEUE_Y 2 * BLOCK_SIZE
#define HOLD_X 1 * BLOCK_SIZE
#define HOLD_Y 2 * BLOCK_SIZE

typedef unsigned char Uint8;
typedef signed char Sint8;
typedef unsigned short Uint16;
typedef unsigned int Uint32;

typedef unsigned char bool;
enum {false,true};

// This array describes the block configuration of a piece, for each shape
// and rotation in a 4x4 grid ordered left to right, then top to bottom
// Indexed: PieceDB[type][flip]
Uint16 PieceDB[7][4] = { // O, I, L, J, S, Z, T
	{0b0000011001100000,0b0000011001100000,0b0000011001100000,0b0000011001100000},
	{0b0100010001000100,0b0000111100000000,0b0010001000100010,0b0000000011110000},
	{0b0110010001000000,0b0000111000100000,0b0100010011000000,0b1000111000000000},
	{0b0100010001100000,0b0000111010000000,0b1100010001000000,0b0010111000000000},
	{0b0110110000000000,0b0100011000100000,0b0000011011000000,0b1000110001000000},
	{0b1100011000000000,0b0010011001000000,0b0000110001100000,0b0100110010000000},
	{0b0100111000000000,0b0100011001000000,0b0000111001000000,0b0100110001000000}
};
// Helper function to single out a block based on x and y position
Uint16 blockmask(int x, int y) { return 0x8000>>(x+y*4); }

Uint32 PieceColor[8] = {
	COLOR_YELLOW, // O - Yellow
	COLOR_CYAN,   // I - Cyan
	COLOR_BLUE,   // J - Blue
	COLOR_ORANGE, // L - Orange
	COLOR_GREEN,  // S - Green
	COLOR_RED,    // Z - Red
	COLOR_PURPLE, // T - Purple
	COLOR_SHADOW  // Shadow
};

// Represents an "instance" of a piece
typedef struct {
	// X and Y position (in blocks) on the stage
	Sint8 x; Sint8 y;
	// Type and flip value to index the PieceDB array
	Uint8 type; Uint8 flip;
} Piece;

// Current game mode (title screen, stage, game over screen, etc)
int gameMode = MODE_STAGE;
// Contains blocks/pieces that have fallen and bacame part of the stage
Uint8 stage[STAGE_W][STAGE_H];
// Random bag used to decide piece order
Uint8 randomBag[7], bagCount = 0;
// Block speed is the falling speed measured in frames between motions
// The block time is the elapsed frames which counts up to block speed
int blockSpeed = INITIAL_SPEED, blockTime = 0;
// Player's stats, score, level, etc
int score = 0, linesCleared = 0, totalLines = 0, level = 1, nextLevel = LINES_PER_LEVEL;
// Current piece controlled by player, held for later, and the queue
Piece piece, hold, queue[5];
// Whether the player held the previous piece, and has held any piece yet
bool holded = false, heldSomething = false;
// Whether game is paused or running. Not running means the game will exit
bool paused = false, running = true;
// True if the player is holding down to soft drop a piece
bool dropping = false;
// Frame count for delayed auto shift, and the direction the piece is being shifted
int autoShift = SHIFT_DELAY, shiftDirection = 0;

void Initialize();
void MoveLeft();
void MoveRight();
bool ValidatePiece(Piece p);
void RotateLeft();
void RotateRight();
void HoldPiece();
bool CheckLock(Piece p);
void DropPiece();
Piece DropShadow(Piece p);
void LockPiece();
void ResetSpeed();
void ClearRow();
void NextPiece();
void GameOver();
void Update();
void Draw();

int main(int argc, char *argv[]) {
	log_open("error.log");
	Initialize();
	log_msg(INFO, "Startup success.\n");
	while(running) {
		Update();
		Draw();
	}
	graphics_quit();
	log_msg(INFO, "Process exited cleanly.\n");
	log_close();
	return 0;
}

// Regenerate the random bag, it contains the next 7 pieces to go in the queue
// It always contains one of each type of tetromino
void FillBag() {
	Uint8 pool[7] = { 0, 1, 2, 3, 4, 5, 6 };
	for(int i = 0; i < 7; i++) {
		int j = rand() % (7 - i);
		randomBag[i] = pool[j];
		for(; j < 6; j++) {
			pool[j] = pool[j+1];
		}
	}
	bagCount = 0;
	log_msgf(TRACE, "FillBag: %hhu, %hhu, %hhu, %hhu, %hhu, %hhu, %hhu\n",
		randomBag[0], randomBag[1], randomBag[2], randomBag[3], 
		randomBag[4], randomBag[5], randomBag[6]);
}

void Initialize() {
	graphics_init(SCREEN_W, SCREEN_H);
	graphics_load_font("data/DejaVuSerif.ttf");
	FillBag();
	piece.type = randomBag[bagCount++];
	piece.flip = 0;
	piece.y = -2;
	piece.x = 3;
	for(int i = 0; i < 5; i++) {
		queue[i].type = randomBag[bagCount++];
		queue[i].flip = 0;
	}
	ResetSpeed();
}

// Move to the left if possible
void MoveLeft() {
	Piece p = piece;
	p.x--;
	if(ValidatePiece(p)) {
		piece = p;
		// Reset timer if next fall will lock
		if(CheckLock(p)) blockTime = 0;
	}
}

// Move to the right if possible
void MoveRight() {
	Piece p = piece;
	p.x++;
	if(ValidatePiece(p)) {
		piece = p;
		// Reset timer if next fall will lock
		if(CheckLock(piece)) blockTime = 0;
	}
}

// Checks if the piece is overlapping with anything
bool ValidatePiece(Piece p) {
	for(int i = 0; i < 4; i++) {
		for(int j = 0; j < 4; j++) {
			int x = p.x + i, y = p.y + j;
			if(PieceDB[p.type][p.flip]&blockmask(i, j)) {
				if (x < 0 || x >= STAGE_W || y >= STAGE_H) return false;
				if (y >= 0 && stage[x][y] > 0) return false;
			}
		}
	}
	return true;
}

bool WallKick(Piece *p) {
	p->x -= 1;
	if(ValidatePiece(*p)) return true;
	p->x += 2;
	if(ValidatePiece(*p)) return true;
	p->x -= 1;
	p->y -= 1;
	if(ValidatePiece(*p)) return true;
	p->y += 1;
	return false;
}

// Rotate to the left if possible
void RotateLeft() {
	Piece p = piece;
	if(p.flip == 0) p.flip = 3; else p.flip--;
	// If rotating makes the piece overlap, try to wall kick
	if(ValidatePiece(p) || WallKick(&p)) {
		piece = p;
		// Reset timer if next fall will lock
		if(CheckLock(piece)) blockTime = 0;
	}
}

// Rotate to the right if possible
void RotateRight() {
	Piece p = piece;
	if(p.flip == 3) p.flip = 0; else p.flip++;
	// If rotating makes the piece overlap, try to wall kick
	if(ValidatePiece(p) || WallKick(&p)) {
		piece = p;
		// Reset timer if next fall will lock
		if(CheckLock(piece)) blockTime = 0;
	}
}

// Switch current and hold block
void HoldPiece() {
	if(holded) return; // Don't hold twice in a row
	piece.x = 3; piece.y = 0;
	if(heldSomething) {
		Piece temp = piece;
		piece = hold;
		hold = temp;
	} else {
		hold = piece;
		heldSomething = true;
		NextPiece();
	}
	holded = true;
}

// Check if piece can be moved down any further
bool CheckLock(Piece p) {
	p.y++;
	return !ValidatePiece(p);
}

// Drop piece to the bottom and lock it
void DropPiece() {
	while (!CheckLock(piece)) {
		piece.y++;
		score += SCORE_HARD_DROP;
	}
	LockPiece();
}

// Returns a shadow to display where the piece will drop
Piece DropShadow(Piece p) {
	while(!CheckLock(p)) p.y++;
	return p;
}

bool DetectTspin(Piece p) {
	return (stage[p.x, p.y] > 0) + (stage[p.x + 2, p.y] > 0) +
		(stage[p.x + 2, p.y + 2] > 0) + (stage[p.x, p.y + 2] > 0) == 3;
}

// Lock piece into stage and spawn the next
void LockPiece() {
	// Push piece data into stage
	for(int i = 0; i < 4; i++) {
		for(int j = 0; j < 4; j++) {
			if(PieceDB[piece.type][piece.flip]&blockmask(i, j)) {
				stage[piece.x+i][piece.y+j] = piece.type+1;
			}
		}
	}
	// Clear any completed rows
	int rows_cleared = 0;
	for(int i = 0; i < STAGE_H; i++) {
		int filled = 0;
		for(int j = 0; j < STAGE_W; j++) if (stage[j][i] > 0) filled++;
		if (filled == STAGE_W) {
			ClearRow(i);
			rows_cleared++;
		}
	}
	// Score rewards
	int reward = 0;
	// 3-corner T-spin
	if(piece.type == 7 && DetectTspin(piece)) {
		switch(rows_cleared) {
			case 0: reward += SCORE_TSPIN * level; break;
			case 1: reward += SCORE_TSPIN_SINGLE * level; break;
			case 2: reward += SCORE_TSPIN_DOUBLE * level; break;
		}
	} else {
		// Immobile (EZ) T-spin
		Piece p = piece;
		if(piece.type == 7 && WallKick(&p)) {
			switch(rows_cleared) {
				case 0: reward += SCORE_EZ_TSPIN * level; break;
				case 1: reward += SCORE_EZ_TSPIN_SINGLE * level; break;
			}
		} else {
			// Rows clear, no T-spin
			switch (rows_cleared) {
				case 1: reward += SCORE_SINGLE * level; break;
				case 2: reward += SCORE_DOUBLE * level; break;
				case 3: reward += SCORE_TRIPLE * level; break;
				case 4: reward += SCORE_TETRIS * level; break;
			}
		}
	}
	score += reward;
	// Update line total and level
	linesCleared += rows_cleared;
	totalLines += rows_cleared;
	nextLevel -= rows_cleared;
	if (nextLevel <= 0) {
		nextLevel += LINES_PER_LEVEL;
		level++;
		ResetSpeed();
	}
	NextPiece();
}

// Adjusts the fall speed based on the current level
void ResetSpeed() {
	if (level <= 5) blockSpeed = INITIAL_SPEED - (level * 5);
	else if (level <= 10) blockSpeed = INITIAL_SPEED - (level * 4) - 5;
	else if (level <= 15) blockSpeed = INITIAL_SPEED - (level * 3) - 10;
	else if (level <= 20) blockSpeed = INITIAL_SPEED - (level * 2) - 15;
	else blockSpeed = INITIAL_SPEED - level - 20;
}

// Clear a row and move down above rows
void ClearRow(int row) {
	for(int i = row; i > 0; i--) {
		for(int j = 0; j < STAGE_W; j++) {
			stage[j][i] = stage[j][i-1];
		}
	}
	for(int j = 0; j < STAGE_W; j++) stage[0][j] = 0;
}

// Shift to the next block in the queue
void NextPiece() {
	piece = queue[0];
	piece.y = -2;
	piece.x = 3;
	for(int i = 0; i < 4; i++) queue[i] = queue[i+1];
	// Grab piece from the bag, refill if it becomes empty
	queue[4].type = randomBag[bagCount++];
	if(bagCount == 7) FillBag();
	holded = false; // Allow player to hold the next piece
	// End the game if the next piece overlaps
	if(!ValidatePiece(piece)) GameOver();
}

void GameOver() {
	gameMode = MODE_GAMEOVER;
}

void DrawPiece(Piece p, int x, int y, bool shadow) {
	// 7 is the shadow color, others match with Piece.type
	int c = shadow ? 7 : p.type;
	graphics_set_color(PieceColor[c]);
	for(int i = 0; i < 4; i++) {
		for(int j = 0; j < 4; j++) {
			if(PieceDB[p.type][p.flip]&blockmask(i, j)) {
				if(p.y + j < 0) continue;
				DrawRectFill(x + i * BLOCK_SIZE + 1, y + j * BLOCK_SIZE + 1, 
					BLOCK_SIZE - 2, BLOCK_SIZE - 2);
			}
		}
	}
}

void MoveDown() {
	if(CheckLock(piece)) {
		LockPiece(piece);
	} else {
		piece.y++;
		if(dropping) score += SCORE_SOFT_DROP;
	}
	blockTime = 0;
}

void ResetGame() {
	for(int i = 0; i < STAGE_W; i++) {
		for(int j = 0; j < STAGE_H; j++) {
			stage[i][j] = 0;
		}
	}
	score = 0;
	level = 0;
	nextLevel = LINES_PER_LEVEL;
	linesCleared = 0;
	totalLines = 0;
	FillBag();
	//piece = randomBag[bagCount++];
	for(int i = 0; i < 5; i++) {
		queue[i].type = randomBag[bagCount++];
	}
	NextPiece();
	heldSomething = false;
	holded = false;
	blockSpeed = INITIAL_SPEED;
	blockTime = 0;
	gameMode = MODE_STAGE;
}

// Relevant update actions when the game is being played
void UpdateStage() {
	if(key.enter && !oldKey.enter) paused = !paused;
	// Don't update the rest if the game is paused
	if(paused) return;
	// Moving left and right
	if(key.left && !oldKey.left) {
		MoveLeft();
		shiftDirection = -1;
		autoShift = SHIFT_DELAY;
	} else if(key.right && !oldKey.right) {
		MoveRight();
		shiftDirection = 1;
		autoShift = SHIFT_DELAY;
	}
	// Delayed Auto Shift
	if(key.right - key.left == shiftDirection) {
		autoShift--;
		if(autoShift == 0) {
			autoShift = SHIFT_SPEED;
			if(key.left) MoveLeft();
			else if(key.right) MoveRight();
		}
	}
	// Rotating block
	if(key.z && !oldKey.z) RotateLeft();
	if(key.x && !oldKey.x) RotateRight();
	if(key.up && !oldKey.up) RotateRight();
	// Drop and Lock
	if(key.space && !oldKey.space) DropPiece();
	// Hold a block and save it for later
	if(key.shift && !oldKey.shift) HoldPiece();
	// If we hold the down key fall faster
	if(key.down && !oldKey.down) {
		blockSpeed = DROP_SPEED;
		dropping = true;
		MoveDown();
	} else if(!key.down && oldKey.down) {
		ResetSpeed();
		dropping = false;
	}
	// Push block down according to speed
	blockTime++;
	if(blockTime >= blockSpeed) {
		// No matter the gravity, always wait at least half a second
		// before locking
		if(CheckLock(piece) && blockSpeed < 30 && !key.down) {
			blockSpeed = LOCK_DELAY;
		} else {
			MoveDown();
		}
	}
}

// Game over screen
void UpdateGameOver() {
	if(key.enter && !oldKey.enter) ResetGame();
}

void Update() {
	// Update keyboard input and events
	// Close the game if the window is closed or escape key is pressed
	if(UpdateInput() || key.esc) running = false;
	switch(gameMode) {
		case MODE_STAGE:
		UpdateStage();
		break;
		case MODE_GAMEOVER:
		UpdateGameOver();
		break;
	}
}

void DrawStage() {
	// Draw the pieces on the stage
	for (int i = 0; i < 10; i++) {
		for (int j = 0; j < 20; j++) {
			if (stage[i][j] == 0) continue;
			int c = stage[i][j] - 1;
			graphics_set_color(PieceColor[c]);
			DrawRectFill(i * BLOCK_SIZE + STAGE_X + 1, 
				j * BLOCK_SIZE + STAGE_Y + 1, BLOCK_SIZE - 2, BLOCK_SIZE - 2);
		}
	}
	// Draw the ghost piece (shadow)
	Piece shadow = DropShadow(piece);
	DrawPiece(shadow, shadow.x * BLOCK_SIZE + STAGE_X, shadow.y * BLOCK_SIZE + STAGE_Y, true);
	// Draw current piece
	DrawPiece(piece, piece.x * BLOCK_SIZE + STAGE_X, piece.y * BLOCK_SIZE + STAGE_Y, false);
	// Queue pieces
	for(int q = 0; q < 5; q++) {
		DrawPiece(queue[q], QUEUE_X, q * (BLOCK_SIZE*4) + QUEUE_Y, false);
	}
	// Hold piece
	if(heldSomething) {
		DrawPiece(hold, HOLD_X, HOLD_Y, false);
	}
}

void DrawGameOver() {
	graphics_set_color(COLOR_RED);
	DrawString("Game Over", STAGE_X, STAGE_Y + 5*BLOCK_SIZE);
}

void Draw() {
	graphics_set_color(COLOR_BLACK);
	// Draw stage background
	DrawRectFill(STAGE_X, STAGE_Y, STAGE_W * BLOCK_SIZE, STAGE_H * BLOCK_SIZE);
	// Queue background
	DrawRectFill(QUEUE_X, QUEUE_Y, BLOCK_SIZE * 4, BLOCK_SIZE * 4 * 5);
	// Hold background
	DrawRectFill(HOLD_X, HOLD_Y, BLOCK_SIZE * 4, BLOCK_SIZE * 4);
	// Game mode specific draw functions
	switch(gameMode) {
		case MODE_STAGE:
		DrawStage();
		break;
		case MODE_GAMEOVER:
		DrawGameOver();
		break;
	}
	graphics_set_color(COLOR_BLACK);
	// Draw the text
	DrawString("Score: ", STAGE_X, 0);
	DrawInt(score, STAGE_X + TextWidth("Score: ") + 96, 0);
	DrawString("Queue", QUEUE_X, 0);
	DrawString("Hold", HOLD_X, 0);
	DrawString("Level:", HOLD_X, HOLD_Y + (5 * BLOCK_SIZE));
	DrawInt(level,       HOLD_X + 64, HOLD_Y + (7 * BLOCK_SIZE));
	DrawString("Next:",  HOLD_X, HOLD_Y + (10 * BLOCK_SIZE));
	DrawInt(nextLevel,   HOLD_X + 64, HOLD_Y + (12 * BLOCK_SIZE));
	DrawString("Total:", HOLD_X, HOLD_Y + (15 * BLOCK_SIZE));
	DrawInt(totalLines,  HOLD_X + 64, HOLD_Y + (17 * BLOCK_SIZE));
	// Wait until frame time and flip the backbuffer
	graphics_flip();
}
