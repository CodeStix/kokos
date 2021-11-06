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

// This should be the same in linker.ld, the kernel will be loaded at this location
#define KERNEL_BASE_ADDRESS (void *)0x100000
#define KERNEL_SIZE 0x100000

extern volatile unsigned long page_table_level4[512];
extern void(cpu_startup16)();
extern unsigned short cpu_startup_increment;
extern void(interrupt_schedule)();
IOApic *ioapic;

int hugepages_supported()
{
    CpuIdResult result = cpu_id(0x80000001);
    return result.edx & CPU_ID_1GB_PAGES_EDX;
}

void interrupt_schedule_handler(InterruptFrame *stack)
{
    Cpu *current_cpu = cpu_get_current();

    // console_print("[schedule] interrupt fired on cpu 0x");
    // console_print_u64(current_cpu->id, 16);
    // console_print(", return to 0x");
    // console_print_u64((unsigned long)stack->instruction_pointer, 16);
    // console_new_line();
    current_cpu->local_apic->end_of_interrupt = 0;
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
    unsigned int x, y;
    console_get_cursor(&x, &y);
    console_set_cursor(60, 24);
    console_print_u64(counter++, 10);
    console_set_cursor(x, y);

    Cpu *cpu = cpu_get_current();
    cpu->local_apic->end_of_interrupt = 0;
}

ATTRIBUTE_INTERRUPT
void interrupt_handle_timer(InterruptFrame *frame)
{
    unsigned int x, y;
    console_get_cursor(&x, &y);
    console_set_cursor(70, 24);
    console_print_u64(counter2++, 10);
    console_set_cursor(x, y);

    // Create blinking cursor
    console_print_char((counter2 & 0b111) < 4 ? '_' : ' ');
    console_set_cursor(x, y);

    Cpu *cpu = cpu_get_current();
    cpu->local_apic->end_of_interrupt = 0;
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
    unsigned long usable_memory = 0;
    for (int i = 0; i * memory_information->entry_size < memory_information->base.size; i++)
    {
        Multiboot2InfoTagMemoryMapEntry *entry = &memory_information->entries[i];
        if (entry->type == 1)
        {
            usable_memory += entry->length;
            if (entry->address + entry->length > max_address)
            {
                max_address = entry->address + entry->length;
            }
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
    console_print_u64(usable_memory, 10);
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
        // Always place the allocation table higher than 1mb, the low memory area can contain data
        if (entry->type == 1 && entry->length >= allocation_table_size && entry->address >= 0x00100000)
        {
            // Move allocation table after the kernel if it overlaps with the kernel
            if (entry->address + allocation_table_size >= KERNEL_BASE_ADDRESS && entry->address < KERNEL_BASE_ADDRESS + KERNEL_SIZE)
            {
                if (KERNEL_BASE_ADDRESS + KERNEL_SIZE >= entry->address + entry->length)
                {
                    // The allocation table does not fit after the kernel, find next spot
                    continue;
                }
                else
                {
                    // Place the allocation directly after the kernel
                    allocation_table_start = KERNEL_BASE_ADDRESS + KERNEL_SIZE;
                }
            }
            else
            {
                // Found a spot
                allocation_table_start = (void *)entry->address;
            }
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

    // Reserve memory for the kernel itself
    memory_physical_reserve(KERNEL_BASE_ADDRESS, KERNEL_SIZE);

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

    console_print("[cpu] initialize cpu context\n");

    cpu_initialize();

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
    if (acpi_validate_sdt(&rsdt->base))
    {
        console_print("[acpi] rsdt checksum does not match!\n");
        return;
    }
    acpi_print_rsdt(rsdt);

    // TODO support XSDT

    AcpiFadt *fadt = (AcpiFadt *)acpi_rsdt_get_table(rsdt, ACPI_FADT_SIGNATURE);
    if (!fadt)
    {
        console_print("[acpi] fatal: FADT not found!\n");
        return;
    }
    if (acpi_validate_sdt(&fadt->base))
    {
        console_print("[acpi] fatal: FADT not valid\n");
        return;
    }

    AcpiDsdt *dsdt = (AcpiDsdt *)fadt->dsdt;
    if (!dsdt)
    {
        console_print("[acpi] fatal: DSDT not found!\n");
        return;
    }
    if (acpi_validate_sdt(&dsdt->base))
    {
        console_print("[acpi] fatal: DSDT not valid\n");
        return;
    }

    unsigned int dsdt_aml_length = dsdt->base.length - sizeof(AcpiSdt);
    console_print("aml code size = ");
    console_print_u32(dsdt_aml_length, 10);
    console_new_line();
    // console_print("aml code = {\n");
    // for (unsigned int i = 0; i < dsdt->base.length; i++)
    // {
    //     // serial_print("0x");
    //     // serial_write(dsdt->aml_code[i]);
    //     serial_write(*((unsigned char *)dsdt + i));
    //     // serial_print_u32(dsdt->aml_code[i], 16);
    //     // serial_print(", ");
    // }
    // console_print("\n}\n");
    // console_new_line();

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
    if (acpi_validate_sdt(&madt->base))
    {
        console_print("[acpi] fatal: MADT not valid\n");
        return;
    }

    if (!apic_check_supported())
    {
        console_print("[acpi] fatal: apic is not supported!\n");
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
    console_print_u32(apic_id, 2);
    console_new_line();
    console_print("[apic] version ");
    console_print_u32(apic_version, 10);
    console_new_line();
    console_print("[apic] max entries ");
    console_print_u32(apic_max_entries, 10);
    console_new_line();

    console_print("[ioapic] looking for io apic in MADT table\n");

    // Iterate all io apic's
    AcpiMadtEntry1IOAPIC *current_ioapic = 0;
    ioapic = 0;
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
    }

    // acpi_print_madt(madt);

    // console_print("[test] triggering interrupt\n");

    console_print("[smp] cpu entry code at 0x");
    console_print_u64(cpu_startup16, 16);
    console_new_line();

    unsigned int cpu_startup_vector = (unsigned long)cpu_startup16 >> 12;
    console_print("[smp] cpu entry code vector ");
    console_print_i32(cpu_startup_vector, 10);
    console_new_line();
    if (cpu_startup_vector > 255)
    {
        console_print("[smp] fatal: cpu entry code is not under the 1mb barrier, cannot start multiple processors\n");
        return;
    }

    AcpiMadtEntry0LocalAPIC *current_processor = 0;
    while (current_processor = acpi_madt_iterate_type(madt, current_processor, ACPI_MADT_TYPE_LOCAL_APIC))
    {
        console_print("[smp] starting processor ");
        console_print_i32(current_processor->processor_id, 10);
        console_print(" and wait");
        console_new_line();

        if (!(current_processor->flags & 0b1))
        {
            console_print("[smp] skipping because the processor enabled flag is 0\n");
            continue;
        }

        if (current_processor->apic_id == apic_id)
        {
            // Skip boot processor
            console_print("[smp] skipping because boot processor\n");
            continue;
        }

        cpu_startup_increment = 0;

        // Send interrupt of type INIT to the other cpu's APIC, using the boot processors APIC, the processor will then wait for the SIPI
        // apic->interrupt_command1 must be written first because the interrupt will be sent when apic->interrupt_command0 has been written to
        apic->interrupt_command1 = current_processor->apic_id << 24;
        apic->interrupt_command0 = (0b1 << 14) | (0b101 << 8);

        cpu_wait_millisecond();

        // Send interrupt of type SIPI (startup interprocessor interrupt) to the other cpu's APIC
        apic->interrupt_command1 = current_processor->apic_id << 24;
        apic->interrupt_command0 = (0b1 << 14) | (0b110 << 8) | cpu_startup_vector;

        cpu_wait_millisecond();

        // Send it one more time to be sure (recommended by Intel), the cpu will ignore it if the first one succeeded
        apic->interrupt_command1 = current_processor->apic_id << 24;
        apic->interrupt_command0 = (0b1 << 14) | (0b110 << 8) | cpu_startup_vector;

        while (!cpu_startup_increment)
        {
            asm volatile("pause");
        }

        console_print("[smp] startup ok\n");
        console_new_line();
    }

    console_print("[smp] started all processors in a halted state\n");

    // Enable APIC
    apic->spurious_interrupt_vector = 0x1FF;

    console_print("[test] create test interrupts\n");
    console_print("[test] interrupt at 0x");
    console_print_u64((unsigned long)interrupt_schedule, 16);
    console_new_line();

    interrupt_register(0x22, interrupt_schedule, INTERRUPT_GATE_TYPE_INTERRUPT);
    apic->timer_initial_count = 1000000;
    apic->timer_divide_config = 0b1010; // 0b1011
    apic->timer_vector = 0x22 | (1 << 17);

    interrupt_register(0x24, interrupt_handle_timer, INTERRUPT_GATE_TYPE_INTERRUPT);
    IOApicEntry timer_entry = {
        .vector = 0x24,
        .destination = apic->id >> 24,
    };
    apic_io_register(ioapic, 2, timer_entry);

    console_print("[test] enable apic\n");

    // Enable apic (0x100) and set spurious interrupt vector to 0xFF

    // console_print("[pci] iterating pci bus\n");
    // pci_scan();
    // console_print("[pci] done\n");

    console_print("[boot] starting keyboard driver\n");
    keyboard_initialize();
    console_print("[boot] starting serial driver\n");
    serial_initialize();

    console_print("[boot] reached end, type something...\n");

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
