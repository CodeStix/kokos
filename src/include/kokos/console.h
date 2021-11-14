#pragma once

typedef enum
{
    CONSOLE_COLOR_BLACK = 0x0,
    CONSOLE_COLOR_BLUE = 0x1,
    CONSOLE_COLOR_GREEN = 0x2,
    CONSOLE_COLOR_CYAN = 0x3,
    CONSOLE_COLOR_RED = 0x4,
    CONSOLE_COLOR_PURPLE = 0x5,
    CONSOLE_COLOR_BROWN = 0x6,
    CONSOLE_COLOR_GRAY = 0x7,
    CONSOLE_COLOR_DARK_GRAY = 0x8,
    CONSOLE_COLOR_LIGHT_BLUE = 0x9,
    CONSOLE_COLOR_LIGHT_GREEN = 0xA,
    CONSOLE_COLOR_LIGHT_CYAN = 0xB,
    CONSOLE_COLOR_LIGHT_RED = 0xC,
    CONSOLE_COLOR_LIGHT_PURPLE = 0xD,
    CONSOLE_COLOR_YELLOW = 0xE,
    CONSOLE_COLOR_WHITE = 0xF,
} ConsoleColor;

#define CONSOLE_VISIBLE_HEIGHT 25
#define CONSOLE_BUFFER_WIDTH 80
#define CONSOLE_BUFFER_HEIGHT 25 * 100

typedef struct
{
    char charater;
    unsigned char color;
} ConsoleVideoChar;

void console_set_color(ConsoleColor foreground, ConsoleColor background);
void console_new_line();
void console_print_char(char c);
void console_set_cursor(unsigned int new_x, unsigned int new_y);
void console_get_cursor(unsigned int *destination_x, unsigned int *destination_y);
void console_clear();
void console_print(const char *str);
void console_print_length(const char *str, unsigned int length);
void console_print_i32(int num, int base);
void console_print_u32(unsigned int num, int base);
void console_print_i64(long num, int base);
void console_print_u64(unsigned long num, int base);
void console_scroll(int amount);

// Prints to the console using a specific format.
// Examples:
//   console_format("the value is %i", 100);
//   console_format("unsigned long value = %ul", 123ul);
//   console_format("binary int = %bi", 7); // prints "binary int = 111"
//   console_format("unsigned long hex = 0x%uhl", 17); // prints "unsigned long hex = 0x11"
void console_format(const char *format, ...);