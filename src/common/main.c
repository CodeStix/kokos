#include "../include/console.h"
#include "../include/util.h"
#include "../include/pci.h"
#include "../include/acpi.h"
#include "../include/memory.h"
#include "../include/keyboard.h"
#include "../include/apic.h"
#include "../include/paging.h"
#include "../include/multiboot2.h"
#include "../include/memory_physical.h"

#define uint8 unsigned char
#define int8 signed char
#define uint16 unsigned short
#define int16 signed short
#define int32 signed int
#define uint32 unsigned int
#define int64 signed long long
#define uint64 unsigned long long

extern void hit_breakpoint();
extern unsigned long page_table_level4[512];
extern int hugepages_supported();

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

    if (!multiboot2_info_available())
    {
        console_print("fatal: multiboot2 boot information is not available\n");
        return;
    }

    Multiboot2InfoTagMemoryMap *memory_information = (Multiboot2InfoTagMemoryMap *)multiboot2_info_get(MULTIBOOT2_TYPE_MEMORY_MAP);
    if (!memory_information)
    {
        console_print("fatal: no memory information available\n");
        return;
    }

    // Find total amount of memory
    unsigned long max_address = 0;
    for (int i = 0; i * memory_information->entry_size < memory_information->base.size; i++)
    {
        Multiboot2InfoTagMemoryMapEntry *entry = &memory_information->entries[i];
        if (entry->type == 1 && entry->address + entry->length > max_address)
        {
            max_address = entry->address + entry->length;
        }

        console_print("memory at 0x");
        console_print_u64(entry->address, 16);
        console_print(" with length 0x");
        console_print_u64(entry->length, 16);
        console_print(" with type ");
        console_print_u32(entry->type, 10);
        console_print(" (0x");
        console_print_u64(max_address, 16);
        console_print(")");
        console_new_line();
    }

    console_print("total usable ram = ");
    console_print_u64(max_address, 10);
    console_print(" from 0x0 ... 0x");
    console_print_u64(max_address, 16);
    console_new_line();

    // Find first memory region that can fit allocation table
    // Each bit (8 per byte) in the allocation will determine if a memory region of 4096 bytes is taken or not
    unsigned long allocation_table_size = max_address / 4096 / 8;

    console_print("allocation_table_size = ");
    console_print_u64(allocation_table_size, 10);
    console_new_line();

    void *allocation_table_start = (void *)0xFFFFFFFF;
    for (int i = 0; i * memory_information->entry_size < memory_information->base.size; i++)
    {
        Multiboot2InfoTagMemoryMapEntry *entry = &memory_information->entries[i];
        if (entry->type == 1 && entry->length >= allocation_table_size)
        {
            allocation_table_start = (void *)entry->address;
            break;
        }
    }

    if (allocation_table_start == (void *)0xFFFFFFFF)
    {
        console_print("fatal: could not find spot for allocation table\n");
        return;
    }

    console_print("allocation table at 0x");
    console_print_u64((unsigned long)allocation_table_start, 16);
    console_new_line();

    memory_physical_initialize(allocation_table_start, max_address);

    // Reserve memory for the allocation table itself
    memory_physical_reserve(allocation_table_start, allocation_table_size);

    // Reserve first megabyte, got problems when allocating here https://wiki.osdev.org/Memory_Map_(x86)
    memory_physical_reserve((void *)0, 0x000FFFFF);

    // Reserve memory for the kernel itself (starting at 0x8000, size ~100kb)
    memory_physical_reserve((void *)0x00100000, 0x100000);

    // Reserve memory for all non-usable (type != 1) memory regions
    for (int i = 0; i * memory_information->entry_size < memory_information->base.size; i++)
    {
        Multiboot2InfoTagMemoryMapEntry *entry = &memory_information->entries[i];
        if (entry->type != 1)
        {
            memory_physical_reserve((void *)entry->address, entry->length);
        }
    }

    // Initialize paging
    paging_initialize(page_table_level4);

    // Identity map whole memory
    if (hugepages_supported())
    {
        console_print("1gb pages supported, using this to identity map memory\n");

        // Identity map whole memory using 1GB huge pages
        for (unsigned long address = 0; address < ALIGN_TO_PREVIOUS(max_address, 0x40000000ul); address += 0x40000000ul)
        {
            paging_map_at((unsigned long *)address, (unsigned long *)address, PAGING_FLAG_1GB);
        }

        console_print("done\n");
    }
    else
    {
        console_print("1gb pages not supported, using 2mb pages to identity map memory\n");

        // The first GB of pages were already identity mapped using 2MB pages in main.asm, start at 1GB
        for (unsigned long address = 0x40000000ul; address < ALIGN_TO_PREVIOUS(max_address, 0x200000ul); address += 0x200000ul)
        {
            paging_map_at((unsigned long *)address, (unsigned long *)address, PAGING_FLAG_2MB);
        }

        console_print("done\n");
    }

    console_print("done\n");
    console_print("mapped 0x04ffe0000: ");
    console_print_u64((unsigned long)paging_get_physical_address((unsigned long *)0x04ffe0000), 16);
    console_new_line();

    for (int i = 0; i < 5; i++)
    {
        console_print("alloc test: 0x");
        void *virt = paging_allocate(0);
        console_print_u64((unsigned long)virt, 16);
        console_print(" -> 0x");
        console_print_u64((unsigned long)paging_get_physical_address(virt), 16);
        console_print("\n");
    }

    console_print("[end]\n");
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

    // acpi_print_rsdt(rsdt);

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
    console_print("address: 0x");
    console_print_u64((unsigned long)apic, 16);
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