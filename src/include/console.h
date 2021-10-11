#pragma once

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
} ConsoleVideoChar;

void console_set_color(CONSOLE_COLOR foreground, CONSOLE_COLOR background);
void console_new_line();
void console_print_char(char c);
void console_set_cursor(unsigned int newX, unsigned int newY);
void console_clear();
void console_print(const char *str);
void console_print_length(const char *str, unsigned int length);
void console_print_i32(int num, int base);
void console_print_u32(unsigned int num, int base);
void console_print_i64(long num, int base);
void console_print_u64(unsigned long num, int base);