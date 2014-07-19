#ifndef TETRIS_GRAPHICS
#define TETRIS_GRAPHICS

#define COLOR_BLACK 0x000000FF

void graphics_init(int x, int y);

void graphics_load_font(const char *filename);

void graphics_quit();

void graphics_flip();

void graphics_set_color(unsigned int color);

void GenerateText(char *string);

void DrawRectFill(int x, int y, int w, int h);

void DrawString(char *string, int x, int y);

int TextWidth(char *string);

void DrawInt(int n, int x, int y);

#endif
