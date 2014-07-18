#include <stdio.h>
#include <stdlib.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include "input.h"

#define STAGE_W 10
#define STAGE_H 20
#define LINES_PER_LEVEL 20
#define INITIAL_SPEED 45
#define BLOCK_SIZE 16
#define LOCK_TIME 50
#define MAX_TEXT 200

#define STAGE_X 6 * BLOCK_SIZE
#define STAGE_Y 2 * BLOCK_SIZE
#define SCREEN_W 22 * BLOCK_SIZE
#define SCREEN_H 24 * BLOCK_SIZE
#define QUEUE_X 17 * BLOCK_SIZE
#define QUEUE_Y 2 * BLOCK_SIZE
#define HOLD_X 1 * BLOCK_SIZE
#define HOLD_Y 2 * BLOCK_SIZE

typedef unsigned char bool;
enum {false,true};

SDL_Window *window;
SDL_Renderer *renderer;

long frameTime;

TTF_Font *font;
struct { char *string; SDL_Texture *texture; } text[MAX_TEXT];
int text_count = 0;

FILE *logfile;

// PieceDB[type][flip]
Uint16 PieceDB[7][4] = { // O, I, L, J, S, Z, T
	{0b0000011001100000,0b0000011001100000,0b0000011001100000,0b0000011001100000},
	{0b0100010001000100,0b0000111100000000,0b0010001000100010,0b0000000011110000},
	{0b0110010001000000,0b0000111000100000,0b0100010011000000,0b1000111000000000},
	{0b0100010001100000,0b0000111010000000,0b1100010001000000,0b0010111000000000},
	{0b0110110000000000,0b0100011000100000,0b0000011011000000,0b1000110001000000},
	{0b1100011000000000,0b0010011001000000,0b0000110001100000,0b0100110010000000},
	{0b0100111000000000,0b0100011001000000,0b0000111001000000,0b0100110001000000}
};

SDL_Color PieceColor[8] = {
	{ 0xFF, 0xFF, 0x00, 0xFF }, // O - Yellow
	{ 0x00, 0xFF, 0xFF, 0xFF }, // I - Cyan
	{ 0x00, 0x00, 0xFF, 0xFF }, // J - Blue
	{ 0xFF, 0xA0, 0x00, 0xFF }, // L - Orange
	{ 0x00, 0xFF, 0x00, 0xFF }, // S - Green
	{ 0xFF, 0x00, 0x00, 0xFF }, // Z - Red
	{ 0xA0, 0x00, 0xFF, 0xFF }, // T - Purple
	{ 0x60, 0x60, 0x60, 0xFF }  // Shadow
};

typedef struct {
	Sint8 x; Sint8 y;
	Uint8 type; Uint8 flip;
} Piece;

Uint8 stage[STAGE_W][STAGE_H];
Uint8 lastBlock = 7;
int timeLocked = 0;
int blockSpeed = INITIAL_SPEED, blockTime = 0;
int score = 0, linesCleared = 0, totalLines = 0, level = 1, nextLevel = LINES_PER_LEVEL;
Piece piece, hold, queue[5];
bool holded = false, heldSomething = false, paused = false, running = true;

void rip(const char *func, const char *err) {
	fprintf(logfile, "%s: %s\n", func, err);
	fclose(logfile);
	exit(1);
}

void Initialize();
void LoadContent();
void UnloadContent();
void GenerateText(char *string);
void DrawRectFill(int x, int y, int w, int h);
void DrawTexture(SDL_Texture *texture, int x, int y);
void DrawString(char *string, int x, int y);
int TextWidth(char *string);
void DrawInt(int n, int x, int y);
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

int main(int argc, char *argv[]) {
	logfile = fopen("error.log", "w");
	Initialize();
	LoadContent();
	frameTime = SDL_GetTicks();
	while(running) {
		Update();
	}
	UnloadContent();
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();
	fprintf(logfile, "Process exited cleanly.\n");
	fclose(logfile);
	return 0;
}

void Initialize() {
	if(SDL_Init(SDL_INIT_VIDEO)==-1) {
		rip("SDL_Init", SDL_GetError());
	}
	if(SDL_CreateWindowAndRenderer(SCREEN_W, SCREEN_H, 0, &window, &renderer) == -1) {
		rip("SDL_CreateWindowAndRenderer", SDL_GetError());
	}
	SDL_SetWindowTitle(window, "Tetris");
	if(TTF_Init()==-1) {
		rip("TTF_Init", TTF_GetError());
	}
	piece.type = rand() % 7;
	piece.flip = 0;
	piece.y = -2;
	piece.x = 3;
	for(int i = 0; i < 5; i++) {
		queue[i].type = rand() % 7;
		queue[i].flip = 0;
	}
	ResetSpeed();
}

void LoadContent() {
	font = TTF_OpenFont("data/DejaVuSerif.ttf", 18);
	if(!font) {
		rip("TTF_OpenFont", TTF_GetError());
	}
}

void GenerateText(char *string) {
	SDL_Color color = { 0, 0, 0 }; // Black
	SDL_Surface *surface;
	if(!(surface = TTF_RenderUTF8_Blended(font, string, color))) {
		fprintf(logfile, "TTF_RenderUTF8_Blended: %s\n", TTF_GetError());
		return;
	}
	SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
	SDL_FreeSurface(surface);
	if(!texture) {
		fprintf(logfile, "SDL_CreateTextureFromSurface: %s\n", SDL_GetError());
		return;
	}
	text[text_count].string = string;
	text[text_count].texture = texture;
	text_count++;
}

// Draw a filled rectangle
void DrawRectFill(int x, int y, int w, int h) {
	SDL_Rect rect = { x, y, w, h };
	SDL_RenderFillRect(renderer, &rect);
}

void DrawTexture(SDL_Texture *texture, int x, int y) {
	SDL_Rect drect = { x, y, 0, 0 };
	SDL_QueryTexture(texture, NULL, NULL, &drect.w, &drect.h);
	SDL_RenderCopy(renderer, texture, NULL, &drect);
}

void DrawTextureRect(SDL_Texture *texture, int x, int y, int w, int h) {
	SDL_Rect drect = { x, y, w, h };
	SDL_RenderCopy(renderer, texture, NULL, &drect);
}

void DrawString(char *string, int x, int y) {
	if(strcmp(string, "") == 0) return;
	for(int i = 0; i < text_count; i++) {
		if(strcmp(string, text[i].string)!=0) continue;
		DrawTexture(text[i].texture, x, y);
		return;
	}
	// If a texture for the string doesn't exist create it
	GenerateText(string);
	DrawTexture(text[text_count-1].texture, x, y);
}

int TextWidth(char *string) {
	if(strcmp(string, "") == 0) return 0;
	for(int i = 0; i < text_count; i++) {
		if(strcmp(string, text[i].string)!=0) continue;
		int width;
		SDL_QueryTexture(text[i].texture, NULL, NULL, &width, NULL);
		return width;
	}
	return 0;
}

void DrawInt(int n, int x, int y) {
	do {
		x -= 12;
		switch(n % 10) {
			case 0: DrawString("0", x, y); break;
			case 1: DrawString("1", x, y); break;
			case 2: DrawString("2", x, y); break;
			case 3: DrawString("3", x, y); break;
			case 4: DrawString("4", x, y); break;
			case 5: DrawString("5", x, y); break;
			case 6: DrawString("6", x, y); break;
			case 7: DrawString("7", x, y); break;
			case 8: DrawString("8", x, y); break;
			case 9: DrawString("9", x, y); break;
		}
		n /= 10;
	} while(n > 0);
}

void UnloadContent() {
	
}

// Move to the left if possible
void MoveLeft() {
	piece.x--;
	if(!ValidatePiece(piece)) piece.x++;
}
// Move to the right if possible
void MoveRight() {
	piece.x++;
	if(!ValidatePiece(piece)) piece.x--;
}
// Checks if the given block has any overlap problems
bool ValidatePiece(Piece p) {
	for(int i = 0; i < 4; i++) {
		for(int j = 0; j < 4; j++) {
			int x = p.x + i, y = p.y + j;
			if(PieceDB[p.type][p.flip]&(0x8000>>(i+j*4))) {
				if (x < 0 || x >= STAGE_W || y >= STAGE_H) return false;
				if (y >= 0 && stage[x][y] > 0) return false;
			}
		}
	}
	return true;
}
// Rotate to the left if possible
void RotateLeft() {
	if(piece.flip == 0) piece.flip = 3; else piece.flip--;
	if(!ValidatePiece(piece)) RotateRight();
}
// Rotate to the right if possible
void RotateRight() {
	if(piece.flip == 3) piece.flip = 0; else piece.flip++;
	if(!ValidatePiece(piece)) RotateLeft();
}
// Switch current and hold block
void HoldPiece() {
	if (!holded) {
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
}
// Check if the block can be moved down any further
bool CheckLock(Piece p) {
	p.y++;
	return !ValidatePiece(p);
}
// Drop block to the bottom and lock it
void DropPiece() {
	while (!CheckLock(piece)) piece.y++;
	LockPiece();
}
// Returns a shadow to display where the block will drop
Piece DropShadow(Piece p) {
	while(!CheckLock(p)) p.y++;
	return p;
}
// Make data from this block part of the stage
void LockPiece() {
	for(int i = 0; i < 4; i++) {
		for(int j = 0; j < 4; j++) {
			if(PieceDB[piece.type][piece.flip]&(0x8000>>(i+j*4))) {
				stage[piece.x+i][piece.y+j] = piece.type+1;
			}
		}
	}
	int rows_cleared = 0;
	for(int i = 0; i < STAGE_H; i++) {
		int filled = 0;
		for(int j = 0; j < STAGE_W; j++) if (stage[j][i] > 0) filled++;
		if (filled == STAGE_W) {
			ClearRow(i);
			rows_cleared++;
		}
	}
	switch (rows_cleared) {
		case 1: score += 100; break;
		case 2: score += 200; break;
		case 3: score += 400; break;
		case 4: score += 800; break;
	}
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

void ResetSpeed() {
	if (level <= 5) blockSpeed = INITIAL_SPEED - (level * 5);
	else if (level <= 10) blockSpeed = INITIAL_SPEED - (level * 4) - 5;
	else if (level <= 15) blockSpeed = INITIAL_SPEED - (level * 3) - 10;
	else if (level <= 20) blockSpeed = INITIAL_SPEED - (level * 2) - 15;
	else blockSpeed = INITIAL_SPEED - level - 20;
}

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
	// Lets make sure we don't get the same piece 6 times in a row
	while(queue[4].type == lastBlock) queue[4].type = rand() % 7;
	lastBlock = queue[4].type;
	blockTime = 0;
	holded = false;
	if(!ValidatePiece(piece)) GameOver();
}

void GameOver() {
	//exit(0);
}

void DrawPiece(Piece p, int x, int y, bool shadow) {
	int c = shadow ? 7 : p.type;
	SDL_SetRenderDrawColor(renderer, PieceColor[c].r, 
		PieceColor[c].g, PieceColor[c].b, PieceColor[c].a);
	for(int i = 0; i < 4; i++) {
		for(int j = 0; j < 4; j++) {
			if(PieceDB[p.type][p.flip]&(0x8000>>(i+j*4))) {
				if(p.y + j < 0) continue;
				DrawRectFill(x + i * BLOCK_SIZE + 1, y + j * BLOCK_SIZE + 1, 
					BLOCK_SIZE - 2, BLOCK_SIZE - 2);
			}
		}
	}
}

void Update() {
	// Input and pause handling
	if(UpdateInput() || key.esc) running = false;
	if(key.enter && !oldKey.enter) paused = !paused;
	// Update block
	if (!paused) {
		// Moving left and right
		if(key.left && !oldKey.left) MoveLeft();
		if(key.right && !oldKey.right) MoveRight();
		// Rotating block
		if(key.z && !oldKey.z) RotateLeft();
		if(key.x && !oldKey.x) RotateRight();
		if(key.up && !oldKey.up) RotateRight();
		// Drop and Lock
		if(key.space && !oldKey.space) DropPiece();
		// Hold a block and save it for later
		if(key.shift && !oldKey.shift) HoldPiece();
		// If we hold down the down key fall faster
		if(blockSpeed > 5 && key.down) {
			blockTime++;
			if(blockTime >= 5) {
				blockTime = 0;
				if(CheckLock(piece)) {
					LockPiece(piece);
				} else piece.y++;
			}
		} else {
			blockTime++;
			if (blockTime >= blockSpeed) {
				blockTime = 0;
				if(CheckLock(piece)) {
					LockPiece(piece);
				} else piece.y++;
			}
		}
	}
	
	// Draw stage
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
	DrawString("Score: ", STAGE_X, 0);
	DrawInt(score, STAGE_X + TextWidth("Score: ") + 96, 0);
	DrawRectFill(STAGE_X, STAGE_Y, STAGE_W * BLOCK_SIZE, STAGE_H * BLOCK_SIZE);
	for (int i = 0; i < 10; i++) {
		for (int j = 0; j < 20; j++) {
			if (stage[i][j] == 0) continue;
			int c = stage[i][j] - 1;
			SDL_SetRenderDrawColor(renderer, PieceColor[c].r,
				PieceColor[c].g, PieceColor[c].b, PieceColor[c].a);
			DrawRectFill(i * BLOCK_SIZE + STAGE_X + 1, 
				j * BLOCK_SIZE + STAGE_Y + 1, BLOCK_SIZE - 2, BLOCK_SIZE - 2);
		}
	}
	// Draw shadow
	Piece shadow = DropShadow(piece);
	DrawPiece(shadow, shadow.x * BLOCK_SIZE + STAGE_X, shadow.y * BLOCK_SIZE + STAGE_Y, true);
	// Draw piece
	DrawPiece(piece, piece.x * BLOCK_SIZE + STAGE_X, piece.y * BLOCK_SIZE + STAGE_Y, false);
	// Queue
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
	DrawString("Queue", QUEUE_X, 0);
	DrawRectFill(QUEUE_X, QUEUE_Y, BLOCK_SIZE * 4, BLOCK_SIZE * 4 * 5);
	for(int q = 0; q < 5; q++) {
		DrawPiece(queue[q], QUEUE_X, q * (BLOCK_SIZE*4) + QUEUE_Y, false);
	}
	// Hold
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
	DrawString("Hold", HOLD_X, 0);
	DrawRectFill(HOLD_X, HOLD_Y, BLOCK_SIZE * 4, BLOCK_SIZE * 4);
	if(heldSomething) {
		DrawPiece(hold, HOLD_X, HOLD_Y, false);
	}
	// Some info
	DrawString("Level:", HOLD_X, HOLD_Y + (5 * BLOCK_SIZE));
	DrawInt(level,       HOLD_X + 64, HOLD_Y + (7 * BLOCK_SIZE));
	DrawString("Next:",  HOLD_X, HOLD_Y + (10 * BLOCK_SIZE));
	DrawInt(nextLevel,   HOLD_X + 64, HOLD_Y + (12 * BLOCK_SIZE));
	DrawString("Total:", HOLD_X, HOLD_Y + (15 * BLOCK_SIZE));
	DrawInt(totalLines,  HOLD_X + 64, HOLD_Y + (17 * BLOCK_SIZE));
	
	SDL_Delay(1000 / 60 - SDL_GetTicks() + frameTime);
	frameTime = SDL_GetTicks();
	SDL_SetRenderDrawColor(renderer, 0, 192, 0, 255);
	SDL_RenderPresent(renderer);
	SDL_RenderClear(renderer);
}
