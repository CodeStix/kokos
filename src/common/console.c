#include "../include/console.h"
#include "../include/util.h"

volatile ConsoleVideoChar *video_memory = (ConsoleVideoChar *)0xb8000;

static unsigned char current_console_color = 0x0f;
static unsigned int x = 0, y = 0;
static int scroll = 0;

static ConsoleVideoChar history[CONSOLE_BUFFER_WIDTH * CONSOLE_BUFFER_HEIGHT];

static void console_update_screen()
{
    for (int iy = 0; iy < CONSOLE_VISIBLE_HEIGHT; iy++)
    {
        for (int ix = 0; ix < CONSOLE_BUFFER_WIDTH; ix++)
        {
            video_memory[ix + iy * CONSOLE_BUFFER_WIDTH] = history[ix + (iy + scroll) * CONSOLE_BUFFER_WIDTH];
        }
    }
}

void console_set_color(ConsoleColor foreground, ConsoleColor background)
{
    current_console_color = (foreground & 0xF) | ((background & 0xF) << 4);
}

void console_new_line()
{
    if (++y >= CONSOLE_BUFFER_HEIGHT)
    {
        console_clear();

        // Shift entire buffer so the new line can fit on the screen
        // for (int iy = 0; iy < CONSOLE_BUFFER_HEIGHT - 1; iy++)
        // {
        //     for (int ix = 0; ix < CONSOLE_BUFFER_WIDTH; ix++)
        //     {
        //         history[ix + iy * CONSOLE_BUFFER_WIDTH] = history[ix + (iy + 1) * CONSOLE_BUFFER_WIDTH];
        //     }
        // }

        // Insert new empty line
        // y = CONSOLE_BUFFER_HEIGHT - 1;
        // for (int ix = 0; ix < CONSOLE_BUFFER_WIDTH; ix++)
        // {
        //     history[ix + y * CONSOLE_BUFFER_WIDTH].charater = 0x0;
        //     history[ix + y * CONSOLE_BUFFER_WIDTH].color = 0x0;
        // }
    }
    x = 0;

    if (y >= scroll + CONSOLE_VISIBLE_HEIGHT && scroll < CONSOLE_BUFFER_HEIGHT - 1)
    {
        // Scroll down
        scroll++;
        console_update_screen();
    }
}

void console_print_char(char c)
{
    // serial_write(c);
    if (c == '\n')
    {
        console_new_line();
    }
    else
    {
        history[x + y * CONSOLE_BUFFER_WIDTH].charater = c;
        history[x + y * CONSOLE_BUFFER_WIDTH].color = current_console_color;
        if (y >= scroll && y < scroll + CONSOLE_BUFFER_HEIGHT)
        {
            video_memory[x + (y - scroll) * CONSOLE_BUFFER_WIDTH].charater = c;
            video_memory[x + (y - scroll) * CONSOLE_BUFFER_WIDTH].color = current_console_color;
        }

        if (++x >= CONSOLE_BUFFER_WIDTH)
        {
            console_new_line();
        }
    }
}

void console_set_cursor(unsigned int new_x, unsigned int new_y)
{
    x = new_x;
    y = new_y + scroll;
}

void console_get_cursor(unsigned int *destination_x, unsigned int *destination_y)
{
    *destination_x = x;
    *destination_y = y - scroll;
}

void console_clear()
{
    for (int ix = 0; ix < CONSOLE_BUFFER_WIDTH; ix++)
    {
        for (int iy = 0; iy < CONSOLE_BUFFER_HEIGHT; iy++)
        {
            history[ix + iy * CONSOLE_BUFFER_WIDTH].charater = 0x0;
            history[ix + iy * CONSOLE_BUFFER_WIDTH].color = 0x0;
        }
    }
    x = 0;
    y = 0;
    scroll = 0;
    console_update_screen();
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

void console_scroll(int amount)
{
    scroll += amount;
    if (scroll < 0)
    {
        scroll = 0;
    }
    else if (scroll >= CONSOLE_BUFFER_HEIGHT)
    {
        scroll = CONSOLE_BUFFER_HEIGHT - 1;
    }
    console_update_screen();
}