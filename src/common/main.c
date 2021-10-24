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
#include "../include/interrupt.h"
#include "../include/cpu.h"
#include "../include/port.h"
#include "../include/serial.h"

#define uint8 unsigned char
#define int8 signed char
#define uint16 unsigned short
#define int16 signed short
#define int32 signed int
#define uint32 unsigned int
#define int64 signed long long
#define uint64 unsigned long long

extern volatile unsigned long page_table_level4[512];

int hugepages_supported()
{
    CpuIdResult result = cpu_id(0x80000001);
    return result.edx & CPU_ID_1GB_PAGES_EDX;
}

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

static volatile unsigned long counter = 0;
static volatile unsigned long counter2 = 0;

ATTRIBUTE_INTERRUPT
void interrupt_handle_test(InterruptFrame *frame)
{
    // console_print("got interrupt ");
    // console_print_u64()
    // console_clear();
    // console_print_u64(frame->instruction_pointer, 16);
    unsigned int x, y;
    console_get_cursor(&x, &y);
    console_set_cursor(40, 24);
    console_print_u64(counter++, 10);
    console_set_cursor(x, y);

    apic_get()->end_of_interrupt = 0;
}

ATTRIBUTE_INTERRUPT
void interrupt_handle_timer(InterruptFrame *frame)
{
    // console_print("got interrupt ");
    // console_print_u64()
    // console_clear();
    // console_print_u64(frame->instruction_pointer, 16);
    unsigned int x, y;
    console_get_cursor(&x, &y);
    console_set_cursor(60, 24);
    console_print_u64(counter2++, 10);
    console_set_cursor(x, y);

    apic_get()->end_of_interrupt = 0;
}

void kernel_main()
{
    console_clear();
    console_set_color(CONSOLE_COLOR_WHITE, CONSOLE_COLOR_BLACK);
    console_print("entered c\n");

    console_print("address of main = 0x");
    console_print_u64((unsigned long)kernel_main, 16);
    console_new_line();

    console_print("address of multiboot2_info = 0x");
    console_print_u64((unsigned long)multiboot2_info, 16);
    console_new_line();

    console_print("[physical memory] initialize physical memory\n");

    if (!multiboot2_info_available())
    {
        console_print("[physical memory] fatal: multiboot2 boot information is not available\n");
        return;
    }

    Multiboot2InfoTagMemoryMap *memory_information = (Multiboot2InfoTagMemoryMap *)multiboot2_info_get(MULTIBOOT2_TYPE_MEMORY_MAP);
    if (!memory_information)
    {
        console_print("[physical memory] fatal: no memory information available\n");
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

        console_print("[physical memory] memory at 0x");
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

    console_print("[physical memory] total usable ram = ");
    console_print_u64(max_address, 10);
    console_print(" from 0x0 ... 0x");
    console_print_u64(max_address, 16);
    console_new_line();

    // Find first memory region that can fit allocation table
    // Each bit (8 per byte) in the allocation will determine if a memory region of 4096 bytes is taken or not
    unsigned long allocation_table_size = max_address / 4096 / 8;

    console_print("[physical memory] allocation_table_size = ");
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
        console_print("[physical memory] fatal: could not find spot for allocation table\n");
        return;
    }

    console_print("[physical memory] allocation table at 0x");
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

    console_print("[paging] initialize paging\n");

    // Initialize paging
    paging_initialize(page_table_level4);

    // Identity map whole memory
    if (hugepages_supported())
    {
        console_print("[paging] 1gb pages supported, using this to identity map memory\n");

        // Identity map whole memory using 1GB huge pages
        for (unsigned long address = 0; address < ALIGN_TO_PREVIOUS(max_address, 0x40000000ul); address += 0x40000000ul)
        {
            paging_map_at((unsigned long *)address, (unsigned long *)address, PAGING_FLAG_1GB | PAGING_FLAG_REPLACE);
        }

        console_print("[paging] done\n");
    }
    else
    {
        console_print("[paging] 1gb pages not supported, using 2mb pages to identity map memory\n");

        // Identity map whole memory using 2MB huge pages
        for (unsigned long address = 0; address < ALIGN_TO_PREVIOUS(max_address, 0x200000ul); address += 0x200000ul)
        {
            paging_map_at((unsigned long *)address, (unsigned long *)address, PAGING_FLAG_2MB | PAGING_FLAG_REPLACE);
        }

        console_print("[paging] done\n");
    }

    for (int i = 0; i < 3; i++)
    {
        console_print("[paging] identity test: 0x");
        int *virt = (int *)0x000fe0000 + i;
        console_print_u64((unsigned long)virt, 16);
        console_print(" -> 0x");
        console_print_u64((unsigned long)paging_get_physical_address(virt), 16);
        console_print("\n");
    }

    for (int i = 0; i < 3; i++)
    {
        console_print("[paging] allocate test: 0x");
        int *virt = paging_allocate(0);
        console_print_u64((unsigned long)virt, 16);
        console_print(" -> 0x");
        console_print_u64((unsigned long)paging_get_physical_address(virt), 16);
        console_print("\n");
        *virt = i * 500;
    }

    console_print("[interrupt] disable pic\n");

    // Disable and remap pic
    apic_disable_pic();

    console_print("[interrupt] initialize\n");

    interrupt_initialize();

    console_print("[acpi] find rsdt\n");

    // Find the root system description table pointer
    AcpiRsdtp *rsdtp = acpi_find_rsdt_pointer();
    if (!rsdtp)
    {
        console_print("[acpi] rsdp not found!\n");
        return;
    }

    acpi_print_rsdtp(rsdtp);

    if (acpi_validate_rsdt_pointer(rsdtp))
    {
        console_print("[acpi] rsdp checksum does not match!\n");
        return;
    }

    AcpiRsdt *rsdt = (AcpiRsdt *)rsdtp->address;
    if (acpi_validate_rsdt(&rsdt->base))
    {
        console_print("[acpi] rsdt checksum does not match!\n");
        return;
    }

    // TODO support XSDT

    if (!apic_check_supported())
    {
        console_print("[acpi] fatal: apic is not supported!\n");
        return;
    }

    AcpiFadt *fadt = (AcpiFadt *)acpi_rsdt_get_table(rsdt, ACPI_FADT_SIGNATURE);
    if (!fadt)
    {
        console_print("[acpi] fatal: FADT not found!\n");
        return;
    }

    // Always false for some reason?
    // if (!(fadt->extended_boot_architecture_flags & 0b10))
    // {
    //     console_print("[acpi] fatal: ps/2 controller not supported!\n");
    //     return;
    // }

    AcpiMadt *madt = (AcpiMadt *)acpi_rsdt_get_table(rsdt, ACPI_MADT_SIGNATURE);
    if (!madt)
    {
        console_print("[acpi] fatal: MADT not found!\n");
        return;
    }

    Apic *apic = (Apic *)paging_map(madt->local_apic_address, 0);

    unsigned char apic_id = (unsigned int)apic->id >> 24;
    unsigned char apic_version = apic->version & 0xFF;
    unsigned char apic_max_entries = (apic->version >> 16) & 0xFF;
    console_print("[apic] physical address: 0x");
    console_print_u64((unsigned long)paging_get_physical_address(apic), 16);
    console_new_line();
    console_print("[apic] virtual address: 0x");
    console_print_u64((unsigned long)apic, 16);
    console_new_line();
    console_print("[apic] size ");
    console_print_u32(sizeof(Apic), 10);
    console_new_line();
    console_print("[apic] spurious register (should be 0xF0) 0x");
    console_print_u64((unsigned char *)(&apic->spurious_interrupt_vector) - (unsigned char *)apic, 16);
    console_new_line();
    console_print("[apic] id ");
    console_print_i32(apic_id, 10);
    console_new_line();
    console_print("[apic] version ");
    console_print_i32(apic_version, 10);
    console_new_line();
    console_print("[apic] max entries ");
    console_print_i32(apic_max_entries, 10);
    console_new_line();

    console_print("[ioapic] looking for io apic in MADT table\n");

    // Iterate all io apic's
    AcpiMadtEntry1IOAPIC *current_ioapic = 0;
    IOApic *ioapic = 0;
    while (current_ioapic = acpi_madt_iterate_type(madt, current_ioapic, ACPI_MADT_TYPE_IO_APIC))
    {
        console_print("[ioapic] id ");
        console_print_u64(current_ioapic->io_apic_id, 10);
        console_print(" at 0x ");
        console_print_u64(current_ioapic->io_apic_address, 16);
        console_print(" with starting irq ");
        console_print_u64(current_ioapic->global_system_interrupt_base, 10);
        console_new_line();

        if (ioapic == 0)
        {
            ioapic = (IOApic *)paging_map(current_ioapic->io_apic_address, PAGING_FLAG_READ | PAGING_FLAG_WRITE);

            console_print("[ioapic] version: ");
            console_print_u64(apic_io_get_version(ioapic), 10);
            console_new_line();

            console_print("[ioapic] entry count: ");
            console_print_u64(apic_io_get_max_entries(ioapic), 10);
            console_new_line();

            console_print("[ioapic] id: ");
            console_print_u64(apic_io_get_id(ioapic), 10);
            console_new_line();
        }

        // unsigned long entry = apic_io_get_entry(ioapic, 1);
        // unsigned long entry = 0x23 | ((unsigned long)apic_id << 56);

        // Use IRQ1 (keyboard)

        // apic_io_set_entry(ioapic, 1, entry);
        // apic_io_set_entry(ioapic, 12, entry);
    }

    acpi_print_madt(madt);

    apic_initialize(apic, ioapic);

    console_print("[test] create test interrupts\n");

    interrupt_register(0x22, interrupt_handle_test, INTERRUPT_GATE_TYPE_INTERRUPT);
    apic->timer_initial_count = 1000;
    apic->timer_divide_config = 0b1010; //0b1011
    apic->timer_vector = 0x22 | (1 << 17);

    interrupt_register(0x24, interrupt_handle_timer, INTERRUPT_GATE_TYPE_INTERRUPT);
    IOApicEntry timer_entry = {
        .vector = 0x24,
        .destination = apic->id >> 24,
    };
    apic_io_register(ioapic, 2, timer_entry);

    console_print("[test] enable apic\n");

    // Enable apic (0x100) and set spurious interrupt vector to 0xFF
    apic->spurious_interrupt_vector = 0x1FF;

    // console_print("[test] triggering interrupt\n");

    keyboard_initialize();
    serial_initialize();

    // int a = 100 / 0;

    // asm volatile("mov rax, 0x777" ::
    //                  : "rax");
    // asm volatile("int3");

    // *((int *)0x123123123123) = 10;
    // console_print_i32(*((int *)0x123123123123), 10); // 100ac9

    // console_print("[test] reached end\n");
    // console_clear();

    // while (1)
    // {
    // }
}
