#include "../include/console.h"
#include "../include/util.h"
#include "../include/pci.h"
#include "../include/acpi.h"
#include "../include/memory.h"
#include "../include/keyboard.h"

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

    RSDTPointer *rsdp = acpi_find_rsdt_pointer();
    if (!rsdp)
    {
        console_print("acpi rsdp not found!\n");
        return;
    }

    console_print("acpi rsdp address: 0x");
    console_print_u64((unsigned long)rsdp, 16);
    console_new_line();

    console_print("acpi rsdp revision: ");
    console_print_u32(rsdp->revision, 10);
    console_new_line();

    console_print("acpi rsdt address: 0x");
    console_print_u32(rsdp->address, 16);
    console_new_line();

    console_print("acpi rsdp oem: ");
    console_print_length(rsdp->oemId, sizeof(rsdp->oemId));
    console_new_line();

    if (acpi_validate_rsdt_pointer(rsdp))
    {
        console_print("acpi rsdp checksum does not match!\n");
        return;
    }

    RSDT *rsdt = (RSDT *)rsdp->address;
    if (acpi_validate_rsdt(&rsdt->header))
    {
        console_print("acpi rsdt checksum does not match!\n");
        return;
    }

    for (int i = 0; i < acpi_rsdt_entry_count(rsdt); i++)
    {
        RSDTHeader *header = (RSDTHeader *)rsdt->tableAddresses[i];
        if (acpi_validate_rsdt(header))
        {
            console_print("invalid ");
        }
        console_print("acpi table ");
        console_print_length(header->signature, 4);
        if (header->signature[0] == 'F' && header->signature[1] == 'A' && header->signature[2] == 'C' && header->signature[3] == 'P')
        {
            FADT *fadt = (FADT *)header;
            console_print(" (fadt.smi_command = 0x");
            console_print_u32(fadt->smi_commandport, 16);
            console_print(") ");
        }

        console_print(" at 0x");
        console_print_u64((unsigned long)header, 16);
        console_new_line("\n");
    }

    // hit_breakpoint();
    // hit_breakpoint();
    // hit_breakpoint();
    // hit_breakpoint();
    // hit_breakpoint();
    // hit_breakpoint();
    // hit_breakpoint();
    // hit_breakpoint();
    // hit_breakpoint();
    // hit_breakpoint();

    // pci_scan();

    keyboard_init();

    console_print("reached end\n");
}