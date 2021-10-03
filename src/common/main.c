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
        console_print_u64(chk, 16);
        console_print(": size = ");
        console_print_i32(chk->size, 10);
        console_print(", next = 0x");
        console_print_u64(chk->next, 16);
        console_print(", prev = 0x");
        console_print_u64(chk->previous, 16);
        console_new_line();
        chk = chk->next;
    }
}

int kernel_main()
{
    console_clear();
    console_set_color(CONSOLE_COLOR_WHITE, CONSOLE_COLOR_BLACK);
    console_print("booting...\n");

    console_print("memoryChunk at 0x");
    console_print_u64(&memoryChunk, 16);
    console_print_char('\n');

    console_print("MemoryChunk size ");
    console_print_i32(sizeof(struct MemoryChunk), 10);
    console_print_char('\n');

    console_print("ptr1 at 0x");
    void *ptr1 = memory_allocate(20);
    console_print_u64(ptr1, 16);
    console_print_char('\n');

    memory_debug();

    console_print("ptr2 at 0x");
    void *ptr2 = memory_allocate(8);
    console_print_u64(ptr2, 16);
    console_print_char('\n');

    memory_debug();

    memory_free(ptr2);

    console_print("ptr3 at 0x");
    void *ptr3 = memory_allocate(18);
    console_print_u64(ptr3, 16);
    console_print_char('\n');

    memory_free(ptr1);
    memory_debug();

    console_print("ptr4 at 0x");
    void *ptr4 = memory_allocate(18);
    console_print_u64(ptr4, 16);
    console_print_char('\n');
    memory_free(ptr4);

    memory_debug();

    console_print("ptr5 at 0x");
    void *ptr5 = memory_allocate(1640);
    console_print_u64(ptr5, 16);
    console_print_char('\n');
    memory_free(ptr5);

    memory_free(ptr3);

    memory_debug();

    // console_print_i32((int)memoryChunk);

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