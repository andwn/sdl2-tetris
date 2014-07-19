#include "graphics.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include "logsys.h"

#define MAX_TEXT 100

SDL_Window *window;
SDL_Renderer *renderer;

long frameTime;

TTF_Font *font;
struct { char *string; SDL_Texture *texture; } text[MAX_TEXT];
int text_count = 0;

void DrawTexture(SDL_Texture *texture, int x, int y);

void graphics_init(int x, int y) {
	if(SDL_Init(SDL_INIT_VIDEO)==-1) {
		log_msgf(FATAL, "SDL_Init: %s\n", SDL_GetError());
	}
	if(SDL_CreateWindowAndRenderer(x, y, 0, &window, &renderer) == -1) {
		log_msgf(FATAL, "SDL_CreateWindowAndRenderer: %s\n", SDL_GetError());
	}
	SDL_SetWindowTitle(window, "Tetris");
	if(TTF_Init()==-1) {
		log_msgf(ERROR, "TTF_Init: %s\n", TTF_GetError());
	}
	frameTime = SDL_GetTicks();
}

void graphics_load_font(const char *filename) {
	font = TTF_OpenFont(filename, 18);
	if(!font) {
		log_msgf(ERROR, "TTF_OpenFont: %s\n", TTF_GetError());
	}
}

// Delete all generated text forcing them to be regenerated next frame
void WipeText() {
	for(int i = 0; i < text_count; i++) {
		SDL_DestroyTexture(text[i].texture);
	}
	text_count = 0;
}

void graphics_quit() {
	TTF_CloseFont(font);
	WipeText();
	TTF_Quit();
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();
}

void graphics_flip() {
	SDL_Delay(1000 / 60 - SDL_GetTicks() + frameTime);
	frameTime = SDL_GetTicks();
	SDL_SetRenderDrawColor(renderer, 0, 192, 0, 255);
	SDL_RenderPresent(renderer);
	SDL_RenderClear(renderer);
}

void graphics_set_color(unsigned int color) {
	SDL_SetRenderDrawColor(renderer, color>>24, color>>16, color>>8, color);
}

void GenerateText(char *string) {
	SDL_Color color = { 0, 0, 0, 255 }; // Black
	SDL_Surface *surface = TTF_RenderUTF8_Blended(font, string, color);
	if(!surface) {
		log_msgf(ERROR, "TTF_RenderUTF8_Blended: %s\n", TTF_GetError());
		return;
	}
	SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
	SDL_FreeSurface(surface);
	if(!texture) {
		log_msgf(ERROR, "SDL_CreateTextureFromSurface: %s\n", SDL_GetError());
		return;
	}
	text[text_count].string = string;
	text[text_count].texture = texture;
	text_count++;
}

void DrawRectFill(int x, int y, int w, int h) {
	SDL_Rect rect = { x, y, w, h };
	SDL_RenderFillRect(renderer, &rect);
}

void DrawTexture(SDL_Texture *texture, int x, int y) {
	SDL_Rect drect = { x, y, 0, 0 };
	SDL_QueryTexture(texture, NULL, NULL, &drect.w, &drect.h);
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
	log_msgf(TRACE, "DrawString: Generating new texture for \"%s\".\n", string);
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
