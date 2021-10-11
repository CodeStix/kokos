#include "../include/console.h"
#include "../include/util.h"
#include "../include/pci.h"
#include "../include/acpi.h"
#include "../include/memory.h"
#include "../include/keyboard.h"
#include "../include/apic.h"

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
    MemoryChunk *chk = &memory_chunk;
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

void test_memory()
{
    void *ptr1 = memory_allocate(400);
    char *str1 = "memory allocation test 1\n";
    memory_copy(str1, ptr1, string_length(str1) + 1);

    void *ptr2 = memory_allocate(400);
    char *str2 = "memory allocation test 2\n";
    memory_copy(str2, ptr2, string_length(str2) + 1);

    ptr1 = memory_resize(ptr1, 6000);

    console_print(ptr1);
    console_print(ptr2);

    memory_free(ptr1);
    memory_free(ptr2);

    // Trigger page fault
    // *((int *)0xffff324234) = 100;

    // Trigger dividy by zero
    // console_print_i32(100 / 0, 10);
}

void kernel_main()
{
    console_set_color(CONSOLE_COLOR_WHITE, CONSOLE_COLOR_BLACK);
    console_print("booting...\n");

    AcpiRsdtp *rsdtp = acpi_find_rsdt_pointer();
    if (!rsdtp)
    {
        console_print("acpi rsdp not found!\n");
        return;
    }

    acpi_print_rsdtp(rsdtp);

    if (acpi_validate_rsdt_pointer(rsdtp))
    {
        console_print("acpi rsdp checksum does not match!\n");
        return;
    }

    AcpiRsdt *rsdt = (AcpiRsdt *)rsdtp->address;
    if (acpi_validate_rsdt(&rsdt->base))
    {
        console_print("acpi rsdt checksum does not match!\n");
        return;
    }

    acpi_print_rsdt(rsdt);

    // TODO support XSDT

    // Step 0: disable normal pic
    // Step 1: check if supported
    // Step 2: write 0x1B to apic base address register msr

    apic_disable_pic();

    if (apic_check_supported())
    {
        console_print("apic is supported!\n");
    }
    else
    {
        console_print("apic is NOT supported!\n");
    }

    // hit_breakpoint();

    // pci_scan();

    keyboard_init();

    console_print("reached end\n");
}