#include "../include/console.h"
#include "../include/util.h"
#include "../include/pci.h"
#include "../include/acpi.h"
#include "../include/memory.h"
#include "../include/keyboard.h"
#include "../include/apic.h"
#include "../include/paging.h"
#include "../include/multiboot2.h"

#define uint8 unsigned char
#define int8 signed char
#define uint16 unsigned short
#define int16 signed short
#define int32 signed int
#define uint32 unsigned int
#define int64 signed long long
#define uint64 unsigned long long

extern void hit_breakpoint();
extern Multiboot2Info *multiboot2_info;

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

    console_print("address of main = 0x");
    console_print_u64((unsigned long)kernel_main, 16);
    console_new_line();

    console_print("address of multiboot2_info = 0x");
    console_print_u64((unsigned long)multiboot2_info, 16);
    console_new_line();

    console_print("multiboot2 total_size = ");
    console_print_u32(multiboot2_info->total_size, 10);
    console_new_line();

    console_print("multiboot2 unused = ");
    console_print_u32(multiboot2_info->unused, 10);
    console_new_line();

    for (int i = sizeof(Multiboot2Info);;)
    {
        Multiboot2InfoTag *tag = (Multiboot2InfoTag *)(((unsigned char *)multiboot2_info) + i);
        if (tag->type == 0)
        {
            break;
        }
        i += ALIGN_TO_NEXT(tag->size, 8);

        console_print("multiboot2 tag of type ");
        console_print_u32(tag->type, 10);
        console_print(" and size ");
        console_print_u32(tag->size, 10);
        console_new_line();
    }

    console_print("end of multiboot");

    while (1)
    {
    }

    // Find the root system description table pointer
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

    if (!apic_check_supported())
    {
        console_print("apic is NOT supported!\n");
        return;
    }

    AcpiMadt *madt = (AcpiMadt *)acpi_rsdt_get_table(rsdt, ACPI_MADT_SIGNATURE);
    if (!madt)
    {
        console_print("madt not found!!!\n");
    }

    Apic *apic = (Apic *)madt->local_apic_address;
    console_print("address test: 0x");
    console_print_u64((unsigned long)paging_get_physical_address(apic), 16);
    console_new_line();
    console_print("apic size ");
    console_print_u32(sizeof(Apic), 10);
    console_new_line();
    console_print("location of spurious 0x");
    console_print_u64((unsigned char *)(&apic->spurious_interrupt_vector) - (unsigned char *)apic, 16);
    console_new_line();
    console_print("apic id ");
    console_print_u32(apic->id, 10);
    console_new_line();
    console_print("apic version ");
    console_print_u32(apic->version, 10);
    console_new_line();

    // hit_breakpoint();

    // pci_scan();

    keyboard_init();

    console_print("reached end\n");
}