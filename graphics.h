#ifndef TETRIS_GRAPHICS
#define TETRIS_GRAPHICS

#define COLOR_BLACK  0x000000FF
#define COLOR_RED    0xFF0000FF
#define COLOR_GREEN  0x00FF00FF
#define COLOR_BLUE   0x0000FFFF
#define COLOR_CYAN   0x00FFFFFF
#define COLOR_YELLOW 0xFFFF00FF
#define COLOR_PURPLE 0xA000FFFF
#define COLOR_ORANGE 0xFFA000FF
#define COLOR_WHITE  0xFFFFFFFF
#define COLOR_SHADOW 0x606060FF

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
