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

void memory_debug()
{
    struct MemoryChunk *chk = &memoryChunk;
    while (chk)
    {
        console_print("0x");
        console_print_u64((unsigned long)chk, 16);
        console_print(": size = ");
        console_print_i32(chk->size, 10);
        console_print(", next = 0x");
        console_print_u64((unsigned long)chk->next, 16);
        console_print(", prev = 0x");
        console_print_u64((unsigned long)chk->previous, 16);
        console_new_line();
        chk = chk->next;
    }
}

int kernel_main()
{
    console_clear();
    console_set_color(CONSOLE_COLOR_WHITE, CONSOLE_COLOR_BLACK);
    console_print("booting...\n");

    void *ptr1 = memory_allocate(400);
    char *str1 = "this is a value\n";
    memory_copy(str1, ptr1, string_length(str1) + 1);

    void *ptr2 = memory_allocate(400);
    char *str2 = "this is the second value\n";
    memory_copy(str2, ptr2, string_length(str2) + 1);

    ptr1 = memory_resize(ptr1, 6000);

    console_print(ptr1);
    console_print(ptr2);

    memory_free(ptr1);
    memory_free(ptr2);

    console_print_i32(100 / 0, 10);
    console_print("end\n");
}