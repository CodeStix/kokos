#include "console.c"
#include "memory.c"
#include "acpi.c"

#define uint8 unsigned char
#define int8 signed char
#define uint16 unsigned short
#define int16 signed short
#define int32 signed int
#define uint32 unsigned int
#define int64 signed long long
#define uint64 unsigned long long

extern void hit_breakpoint();

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

void kernel_main()
{
    console_set_color(CONSOLE_COLOR_WHITE, CONSOLE_COLOR_BLACK);
    console_print("booting...\n");

    struct RSDTPointer *rsdp = acpi_find_rsdt_pointer();
    if (!rsdp)
    {
        console_print("rsdp not found!\n");
        return;
    }

    console_print("rsdp address: 0x");
    console_print_u64((unsigned long)rsdp, 16);
    console_new_line();

    console_print("rsdp revision: ");
    console_print_u32(rsdp->revision, 10);
    console_new_line();

    console_print("rsdt address: 0x");
    console_print_u32(rsdp->address, 16);
    console_new_line();

    console_print("rsdp oem: ");
    console_print_length(rsdp->oemId, sizeof(rsdp->oemId));
    console_new_line();

    if (acpi_validate_rsdt_pointer(rsdp))
    {
        console_print("rsdp checksum does not match!\n");
        return;
    }

    struct RSDT *rsdt = (struct RSDT *)rsdp->address;
    if (acpi_validate_rsdt(&rsdt->header))
    {
        console_print("rsdt checksum does not match!\n");
        return;
    }

    void *ptr1 = memory_allocate(400);
    char *str1 = "this is a value\n";
    memory_copy(str1, ptr1, string_length(str1) + 1);

    void *ptr2 = memory_allocate(400);
    char *str2 = "this is the second value\n";
    memory_copy(str2, ptr2, string_length(str2) + 1);

    ptr1 = memory_resize(ptr1, 6000);

    console_print(ptr1);

    hit_breakpoint();

    console_print(ptr2);

    memory_free(ptr1);
    memory_free(ptr2);

    // Trigger page fault
    // *((int *)0xffff324234) = 100;

    // Trigger dividy by zero
    // console_print_i32(100 / 0, 10);

    console_print("end\n");
}