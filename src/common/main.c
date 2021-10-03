#include "console.c"

// #define u8 unsigned char
// #define i8 char
// #define i16 short
// #define u16 unsigned short
// #define i32 int
// #define u32 unsigned int
// #define i64 long long
// #define u64 unsigned long long

int kernel_main()
{
    console_clear();
    console_set_color(CONSOLE_COLOR_WHITE, CONSOLE_COLOR_BLACK);
    console_print("booting...\n");

    console_set_color(CONSOLE_COLOR_WHITE, CONSOLE_COLOR_CYAN);
    for (int i = 0; i < 256; i++)
    {
        console_print_char(i);
    }
}