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
#define INITIAL_SPEED 60
// Gravity for soft drop when player holds the down button
#define DROP_SPEED 4
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

// Function prototypes and order
void initialize();
void reset_game();
void fill_random_bag();
void move_piece_left();
void move_piece_right();
void move_piece_down();
void rotate_piece_left();
void rotate_piece_right();
void hard_drop();
void hold_piece();
void lock_piece();
bool validate_piece(Piece p);
bool check_lock(Piece p);
bool detect_tspin(Piece p);
bool wall_kick(Piece *p);
Piece ghost_piece(Piece p);
void reset_speed();
void clear_row();
void next_piece();
void update();
void update_stage();
void update_game_over();
void draw();
void draw_piece(Piece p, int x, int y, bool shadow);
void draw_stage();
void draw_game_over();

// Entry point, args are unused but SDL complains if they are missing
int main(int argc, char *argv[]) {
	log_open("error.log");
	initialize();
	log_msgf(INFO, "Startup success.\n");
	while(running) {
		update();
		draw();
	}
	graphics_quit();
	log_msgf(INFO, "Process exited cleanly.\n");
	log_close();
	return 0;
}

// Create the game window and start stuff
void initialize() {
	graphics_init(SCREEN_W, SCREEN_H);
	graphics_load_font("data/DejaVuSerif.ttf");
	reset_game();
}

// Put values back to their defaults and start over
void reset_game() {
	// Clear the stage
	for(int i = 0; i < STAGE_W; i++) {
		for(int j = 0; j < STAGE_H; j++) {
			stage[i][j] = 0;
		}
	}
	// reset bag, queue, piece
	fill_random_bag();
	piece.type = randomBag[bagCount++];
	piece.flip = 0;
	piece.y = -2;
	piece.x = 3;
	for(int i = 0; i < 5; i++) {
		queue[i].type = randomBag[bagCount++];
		queue[i].flip = 0;
	}
	// Default values
	heldSomething = false;
	holded = false;
	score = 0;
	level = 0;
	nextLevel = LINES_PER_LEVEL;
	linesCleared = 0;
	totalLines = 0;
	reset_speed();
	next_piece();
	gameMode = MODE_STAGE;
}

// Regenerate the random bag, it contains the next 7 pieces to go in the queue
// It always contains one of each type of tetromino
void fill_random_bag() {
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

// Move to the left if possible
void move_piece_left() {
	Piece p = piece;
	p.x--;
	if(validate_piece(p)) {
		piece = p;
		// Reset timer if next fall will lock
		if(check_lock(p)) blockTime = 0;
	}
}

// Move to the right if possible
void move_piece_right() {
	Piece p = piece;
	p.x++;
	if(validate_piece(p)) {
		piece = p;
		// Reset timer if next fall will lock
		if(check_lock(piece)) blockTime = 0;
	}
}

// Move down or lock
void move_piece_down() {
	if(check_lock(piece)) {
		lock_piece(piece);
	} else {
		piece.y++;
		if(dropping) score += SCORE_SOFT_DROP;
	}
	blockTime = 0;
}

// Rotate to the left if possible
void rotate_piece_left() {
	Piece p = piece;
	if(p.flip == 0) p.flip = 3; else p.flip--;
	// If rotating makes the piece overlap, try to wall kick
	if(validate_piece(p) || wall_kick(&p)) {
		piece = p;
		// Reset timer if next fall will lock
		if(check_lock(piece)) blockTime = 0;
	}
}

// Rotate to the right if possible
void rotate_piece_right() {
	Piece p = piece;
	if(p.flip == 3) p.flip = 0; else p.flip++;
	// If rotating makes the piece overlap, try to wall kick
	if(validate_piece(p) || wall_kick(&p)) {
		piece = p;
		// Reset timer if next fall will lock
		if(check_lock(piece)) blockTime = 0;
	}
}

// Drop piece to the bottom and lock it
void hard_drop() {
	while (!check_lock(piece)) {
		piece.y++;
		score += SCORE_HARD_DROP;
	}
	lock_piece();
}

// Switch current and hold block
void hold_piece() {
	if(holded) return; // Don't hold twice in a row
	piece.x = 3; piece.y = 0;
	if(heldSomething) {
		Piece temp = piece;
		piece = hold;
		hold = temp;
	} else {
		hold = piece;
		heldSomething = true;
		next_piece();
	}
	holded = true;
}

// Lock piece into stage and spawn the next
void lock_piece() {
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
			clear_row(i);
			rows_cleared++;
		}
	}
	// Score rewards
	int reward = 0;
	// 3-corner T-spin
	if(piece.type == 7 && detect_tspin(piece)) {
		switch(rows_cleared) {
			case 0: reward += SCORE_TSPIN * level; break;
			case 1: reward += SCORE_TSPIN_SINGLE * level; break;
			case 2: reward += SCORE_TSPIN_DOUBLE * level; break;
		}
	} else {
		// Immobile (EZ) T-spin
		Piece p = piece;
		if(piece.type == 7 && wall_kick(&p)) {
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
	}
	next_piece();
}

// Checks if the piece is overlapping with anything
bool validate_piece(Piece p) {
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

// Check if piece can be moved down any further
bool check_lock(Piece p) {
	p.y++;
	return !validate_piece(p);
}

bool detect_tspin(Piece p) {
	return (stage[p.x, p.y] > 0) + (stage[p.x + 2, p.y] > 0) +
		(stage[p.x + 2, p.y + 2] > 0) + (stage[p.x, p.y + 2] > 0) == 3;
}

// Try to push the piece out of an obstacles way
bool wall_kick(Piece *p) {
	// Left
	p->x -= 1;
	if(validate_piece(*p)) return true;
	// Right
	p->x += 2;
	if(validate_piece(*p)) return true;
	// Up
	p->x -= 1;
	p->y -= 1;
	if(validate_piece(*p)) return true;
	// Unable to wall kick, return piece to the way it was
	p->y += 1;
	return false;
}

// Returns a shadow to display where the piece will drop
Piece ghost_piece(Piece p) {
	while(!check_lock(p)) p.y++;
	return p;
}

// Adjusts the fall speed based on the current level
void reset_speed() {
	blockSpeed = INITIAL_SPEED - (level * 5);
	if(blockSpeed < DROP_SPEED) blockSpeed = DROP_SPEED;
}

// Clear a row and move down above rows
void clear_row(int row) {
	for(int i = row; i > 0; i--) {
		for(int j = 0; j < STAGE_W; j++) {
			stage[j][i] = stage[j][i-1];
		}
	}
	for(int j = 0; j < STAGE_W; j++) stage[0][j] = 0;
}

// Shift to the next block in the queue
void next_piece() {
	piece = queue[0];
	piece.y = -2;
	piece.x = 3;
	for(int i = 0; i < 4; i++) queue[i] = queue[i+1];
	// Grab piece from the bag, refill if it becomes empty
	queue[4].type = randomBag[bagCount++];
	if(bagCount == 7) fill_random_bag();
	holded = false; // Allow player to hold the next piece
	// End the game if the next piece overlaps
	if(!validate_piece(piece)) gameMode = MODE_GAMEOVER;
	reset_speed();
}

// Main update, handles events and calls relevant game mode update function
void update() {
	// Update keyboard input and events
	// Close the game if the window is closed or escape key is pressed
	if(input_update() || key.esc) running = false;
	switch(gameMode) {
		case MODE_STAGE:
		update_stage();
		break;
		case MODE_GAMEOVER:
		update_game_over();
		break;
	}
}

// Update actions when the game is being played
void update_stage() {
	if(key.enter && !oldKey.enter) paused = !paused;
	// Don't update the rest if the game is paused
	if(paused) return;
	// Moving left and right
	if(key.left && !oldKey.left) {
		move_piece_left();
		shiftDirection = -1;
		autoShift = SHIFT_DELAY;
	} else if(key.right && !oldKey.right) {
		move_piece_right();
		shiftDirection = 1;
		autoShift = SHIFT_DELAY;
	}
	// Delayed Auto Shift
	if(key.right - key.left == shiftDirection) {
		autoShift--;
		if(autoShift == 0) {
			autoShift = SHIFT_SPEED;
			if(key.left) move_piece_left();
			else if(key.right) move_piece_right();
		}
	}
	// Rotating block
	if(key.z && !oldKey.z) rotate_piece_left();
	if(key.x && !oldKey.x) rotate_piece_right();
	if(key.up && !oldKey.up) rotate_piece_right();
	// Drop and Lock
	if(key.space && !oldKey.space) hard_drop();
	// Hold a block and save it for later
	if(key.shift && !oldKey.shift) hold_piece();
	// If we hold the down key fall faster
	if(key.down && !oldKey.down) {
		blockSpeed = DROP_SPEED;
		dropping = true;
		move_piece_down();
	} else if(!key.down && oldKey.down) {
		reset_speed();
		dropping = false;
	}
	// Push block down according to speed
	blockTime++;
	if(blockTime >= blockSpeed) {
		// No matter the gravity, always wait at least half a second
		// before locking
		if(!check_lock(piece) || blockTime >= LOCK_DELAY || key.down) {
			move_piece_down();
		}
	}
}

// Game over screen
void update_game_over() {
	if(key.enter && !oldKey.enter) reset_game();
}

void draw() {
	graphics_set_color(COLOR_BLACK);
	// Draw stage background
	graphics_draw_rect(STAGE_X, STAGE_Y, STAGE_W * BLOCK_SIZE, STAGE_H * BLOCK_SIZE);
	// Queue background
	graphics_draw_rect(QUEUE_X, QUEUE_Y, BLOCK_SIZE * 4, BLOCK_SIZE * 4 * 5);
	// Hold background
	graphics_draw_rect(HOLD_X, HOLD_Y, BLOCK_SIZE * 4, BLOCK_SIZE * 4);
	// Game mode specific draw functions
	switch(gameMode) {
		case MODE_STAGE:
		draw_stage();
		break;
		case MODE_GAMEOVER:
		draw_game_over();
		break;
	}
	graphics_set_color(COLOR_BLACK);
	// Draw the text
	graphics_draw_string("Score: ", STAGE_X, 0);
	graphics_draw_int(score, STAGE_X + graphics_string_width("Score: ") + 96, 0);
	graphics_draw_string("Queue", QUEUE_X, 0);
	graphics_draw_string("Hold", HOLD_X, 0);
	graphics_draw_string("Level:", HOLD_X, HOLD_Y + (5 * BLOCK_SIZE));
	graphics_draw_int(level,       HOLD_X + 64, HOLD_Y + (7 * BLOCK_SIZE));
	graphics_draw_string("Next:",  HOLD_X, HOLD_Y + (10 * BLOCK_SIZE));
	graphics_draw_int(nextLevel,   HOLD_X + 64, HOLD_Y + (12 * BLOCK_SIZE));
	graphics_draw_string("Total:", HOLD_X, HOLD_Y + (15 * BLOCK_SIZE));
	graphics_draw_int(totalLines,  HOLD_X + 64, HOLD_Y + (17 * BLOCK_SIZE));
	// Wait until frame time and flip the backbuffer
	graphics_flip();
}

void draw_piece(Piece p, int x, int y, bool shadow) {
	// 7 is the shadow color, others match with Piece.type
	int c = shadow ? 7 : p.type;
	graphics_set_color(PieceColor[c]);
	for(int i = 0; i < 4; i++) {
		for(int j = 0; j < 4; j++) {
			if(PieceDB[p.type][p.flip]&blockmask(i, j)) {
				if(p.y + j < 0) continue;
				graphics_draw_rect(x + i * BLOCK_SIZE + 1, y + j * BLOCK_SIZE + 1, 
					BLOCK_SIZE - 2, BLOCK_SIZE - 2);
			}
		}
	}
}

void draw_stage() {
	// Draw the pieces on the stage
	for (int i = 0; i < 10; i++) {
		for (int j = 0; j < 20; j++) {
			if (stage[i][j] == 0) continue;
			int c = stage[i][j] - 1;
			graphics_set_color(PieceColor[c]);
			graphics_draw_rect(i * BLOCK_SIZE + STAGE_X + 1, 
				j * BLOCK_SIZE + STAGE_Y + 1, BLOCK_SIZE - 2, BLOCK_SIZE - 2);
		}
	}
	// Draw the ghost piece (shadow)
	Piece shadow = ghost_piece(piece);
	draw_piece(shadow, shadow.x * BLOCK_SIZE + STAGE_X, shadow.y * BLOCK_SIZE + STAGE_Y, true);
	// Draw current piece
	draw_piece(piece, piece.x * BLOCK_SIZE + STAGE_X, piece.y * BLOCK_SIZE + STAGE_Y, false);
	// Queue pieces
	for(int q = 0; q < 5; q++) {
		draw_piece(queue[q], QUEUE_X, q * (BLOCK_SIZE*4) + QUEUE_Y, false);
	}
	// Hold piece
	if(heldSomething) {
		draw_piece(hold, HOLD_X, HOLD_Y, false);
	}
}

void draw_game_over() {
	graphics_set_color(COLOR_RED);
	graphics_draw_string("Game Over", STAGE_X, STAGE_Y + 5*BLOCK_SIZE);
}
