#include "../include/console.h"
#include "../include/util.h"

volatile ConsoleVideoChar *video_memory = (ConsoleVideoChar *)0xb8000;

static unsigned char current_console_color = 0x0f;
static unsigned int x = 0, y = 0, w = 80, h = 25;

static ConsoleVideoChar history[40000];

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

void console_set_cursor(unsigned int new_x, unsigned int new_y)
{
    x = new_x;
    y = new_y;
}

void console_get_cursor(unsigned int *destination_x, unsigned int *destination_y)
{
    *destination_x = x;
    *destination_y = y;
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
    char dest[sizeof(int) * 8 + 1];
    convert_i32_string(dest, num, base);
    console_print(dest);
}

void console_print_u32(unsigned int num, int base)
{
    char dest[sizeof(unsigned int) * 8 + 1];
    convert_u32_string(dest, num, base);
    console_print(dest);
}

void console_print_i64(long num, int base)
{
    char dest[sizeof(long) * 8 + 1];
    convert_i64_string(dest, num, base);
    console_print(dest);
}

void console_print_u64(unsigned long num, int base)
{
    char dest[sizeof(unsigned long) * 8 + 1];
    convert_u64_string(dest, num, base);
    console_print(dest);
}