#include "util.c"

#define CONSOLE_COLOR unsigned char
#define CONSOLE_COLOR_BLACK (CONSOLE_COLOR)0x0
#define CONSOLE_COLOR_BLUE (CONSOLE_COLOR)0x1
#define CONSOLE_COLOR_GREEN (CONSOLE_COLOR)0x2
#define CONSOLE_COLOR_CYAN (CONSOLE_COLOR)0x3
#define CONSOLE_COLOR_RED (CONSOLE_COLOR)0x4
#define CONSOLE_COLOR_PURPLE (CONSOLE_COLOR)0x5
#define CONSOLE_COLOR_BROWN (CONSOLE_COLOR)0x6
#define CONSOLE_COLOR_GRAY (CONSOLE_COLOR)0x7
#define CONSOLE_COLOR_DARK_GRAY (CONSOLE_COLOR)0x8
#define CONSOLE_COLOR_LIGHT_BLUE (CONSOLE_COLOR)0x9
#define CONSOLE_COLOR_LIGHT_GREEN (CONSOLE_COLOR)0xA
#define CONSOLE_COLOR_LIGHT_CYAN (CONSOLE_COLOR)0xB
#define CONSOLE_COLOR_LIGHT_RED (CONSOLE_COLOR)0xC
#define CONSOLE_COLOR_LIGHT_PURPLE (CONSOLE_COLOR)0xD
#define CONSOLE_COLOR_YELLOW (CONSOLE_COLOR)0xE
#define CONSOLE_COLOR_WHITE (CONSOLE_COLOR)0xF

typedef struct
{
    char charater;
    unsigned char color;
} VideoChar;

volatile VideoChar *video_memory = (VideoChar *)0xb8000;

unsigned char current_console_color = 0x0f;
unsigned int x = 0, y = 0, w = 80, h = 25;

void console_set_color(CONSOLE_COLOR foreground, CONSOLE_COLOR background)
{
    current_console_color = foreground | (background << 4);
}

void console_new_line()
{
    if (++y >= h)
    {
        for (int iy = 0; iy < h - 1; iy++)
        {
            for (int ix = 0; ix < w; ix++)
            {
                video_memory[ix + iy * w] = video_memory[ix + (iy + 1) * w];
            }
        }
        y = h - 1;
        for (int ix = 0; ix < w; ix++)
        {
            video_memory[ix + y * w].charater = 0x0;
            video_memory[ix + y * w].color = 0x0;
        }
    }
    x = 0;
}

void console_print_char(char c)
{
    if (c == '\n')
    {
        console_new_line();
    }
    else
    {
        video_memory[x + y * w].charater = c;
        video_memory[x + y * w].color = current_console_color;
        if (++x >= w)
        {
            console_new_line();
        }
    }
}

void console_set_cursor(unsigned int newX, unsigned int newY)
{
    x = newX;
    y = newY;
}

void console_clear()
{
    for (int ix = 0; ix < w; ix++)
    {
        for (int iy = 0; iy < h; iy++)
        {
            video_memory[ix + iy * w].charater = 0x0;
            video_memory[ix + iy * w].color = 0x0;
        }
    }
    x = 0;
    y = 0;
}

void console_print(const char *str)
{
    while (*str)
    {
        console_print_char(*str);
        str++;
    }
}

void console_print_length(const char *str, unsigned int length)
{
    for (unsigned int i = 0; i < length; i++)
    {
        console_print_char(str[i]);
    }
}

void console_print_i32(int num, int base)
{
    char dest[11]; // 11 = max length of 32 bit integer with minus
    convert_i32_string(dest, num, base);
    console_print(dest);
}

void console_print_u32(unsigned int num, int base)
{
    char dest[10]; // 10 = max length of 32 bit integer
    convert_u32_string(dest, num, base);
    console_print(dest);
}

void console_print_i64(long num, int base)
{
    char dest[21];
    convert_i64_string(dest, num, base);
    console_print(dest);
}

void console_print_u64(unsigned long num, int base)
{
    char dest[20];
    convert_u64_string(dest, num, base);
    console_print(dest);
}