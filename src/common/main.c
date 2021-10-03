#include "console.c"
#include "memory.c"

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
    console_print_number(123123);

    // console_set_color(CONSOLE_COLOR_WHITE, CONSOLE_COLOR_CYAN);
    // for (int i = 0; i < 256; i++)
    // {
    //     console_print_char(i);
    // }

    // console_set_color(CONSOLE_COLOR_WHITE, CONSOLE_COLOR_BLACK);

    // console_clear();
    // for (int i = 0; i < 30; i++)
    // {
    //     console_print("line ");
    //     console_print_char('0' + i);
    //     console_print_char('\n');
    // }
    // console_print("asdfasdf\n");
    // console_print("bottom\n");
    // console_print("booting...\n");
}