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

void graphics_draw_rect(int x, int y, int w, int h);

void graphics_draw_string(char *string, int x, int y);

int graphics_string_width(char *string);

void graphics_draw_int(int n, int x, int y);

#endif
