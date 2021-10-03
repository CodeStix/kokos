#include "console.c"
#include "memory.c"

#define uint8 unsigned char
#define int8 signed char
#define uint16 unsigned short
#define int16 signed short
#define int32 signed int
#define uint32 unsigned int
#define int64 signed long long
#define uint64 unsigned long long

// https://wiki.osdev.org/Interrupt_Descriptor_Table
// struct Idt64Entry
// {
//     uint16 offset1;  // offset bits 0..15
//     uint16 selector; // A code segment selector in GDT or LDT
//     uint8 ist;       // Bits 0..2 holds Interrupt Stack Table offset, rest of bits zero.
//     uint8 type;      // Type and attributes
//     uint16 offset2;  // Offset bits 16..31
//     uint32 offset3;  // Offset bits 32..63
//     uint32 reserved; // Reserved
// };

// extern struct Idt64Entry idt[256];

// void kernel_set_interrupt(unsigned char index, void *isr)
// {
//     uint8 privilegeLevel = 0;
//     uint8 type = 0b1111; // 0b1111 (trap gate) or 0b1110 (interrupt gate)
//     idt[index].offset1 = (uint64)isr;
//     idt[index].offset2 = (uint64)isr >> 16;
//     idt[index].offset2 = (uint64)isr >> 32;
//     idt[index].selector = 8; // code_segment in gdt
//     idt[index].ist = 0;
//     idt[index].type = (1 << 7) | (privilegeLevel << 5) | type;
// }

// void kernel_clear_interrupt(unsigned char index)
// {
//     idt[index].type = 0; // Will clear 'present' bit
// }

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