#include "kokos/console.h"
#include "kokos/util.h"
#include "kokos/pci.h"
#include "kokos/acpi.h"
#include "kokos/memory.h"
#include "kokos/keyboard.h"
#include "kokos/apic.h"
#include "kokos/paging.h"
#include "kokos/multiboot2.h"
#include "kokos/memory_physical.h"
#include "kokos/idt.h"
#include "kokos/cpu.h"
#include "kokos/port.h"
#include "kokos/serial.h"
#include "kokos/scheduler.h"

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
extern GlobalDescriptor gdt64[];
unsigned short cpu_startup_increment = 0;
unsigned short cpu_startup_done = 0;

unsigned long max_memory_address;
IOApic *ioapic;

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
void interrupt_handle_test(IdtFrame *frame)
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
void interrupt_handle_timer(IdtFrame *frame)
{
    // unsigned int x, y;
    // console_get_cursor(&x, &y);
    // console_set_cursor(70, 24);
    // console_print_u64(counter2++, 10);
    // console_set_cursor(x, y);

    // // Create blinking cursor
    // console_print_char((counter2 & 0b111) < 4 ? '_' : ' ');
    // console_set_cursor(x, y);

    Cpu *cpu = cpu_get_current();
    cpu->local_apic->end_of_interrupt = 0;
}

void console_debug(const char *str, unsigned long value, int base)
{
    console_print(str);
    console_print_u64(value, base);
    console_new_line();
}

void test_program()
{
    console_print("[program1] start\n");
    unsigned long counter = 0;
    while (1)
    {
        console_print("[program1] add counter = ");
        console_print_u64(counter, 10);
        console_new_line();
        for (int i = 0; i < 100; i++)
            cpu_wait_millisecond();
        counter++;
    }
}

void test_program2()
{
    console_print("[program2] start\n");
    unsigned long counter = 1;
    while (1)
    {
        console_print("[program2] mul counter = ");
        console_print_u64(counter, 10);
        console_new_line();
        for (int i = 0; i < 100; i++)
            cpu_wait_millisecond();

        counter *= 2;
        if (counter >= 1ul << 63)
        {
            counter = 1;
        }
    }
}

void test_program3()
{
    console_print("[program3] start\n");
    unsigned long counter = 0;
    while (1)
    {
        console_print("[program3] add123 counter = ");
        console_print_u64(counter, 10);
        console_new_line();
        for (int i = 0; i < 100; i++)
            cpu_wait_millisecond();
        counter += 123;
    }
}

void gdt_debug()
{
    for (int i = 0; i < 3; i++)
    {
        GlobalDescriptor entry = gdt64[i];
        if (!entry.present)
            continue;
        console_print("[gdt] entry #");
        console_print_u64(i, 10);
        console_print(" base=0x");
        console_print_u64(entry.base1 | (entry.base2 << 16) | (entry.base3 << 24), 16);
        console_print(" limit=0x");
        console_print_u64(entry.limit1 | (entry.limit2 << 16), 16);
        console_print(" access=");
        if (entry.present)
            console_print("P");
        console_print_u32((unsigned int)entry.privilege, 10);
        if (entry.type)
            console_print("T");
        if (entry.executable)
            console_print("E");
        if (entry.direction_conforming)
            console_print("D");
        if (entry.read_write)
            console_print("W");
        if (entry.accessed)
            console_print("a");
        console_print(" flags=");
        if (entry.granularity)
            console_print("G");
        if (entry.size)
            console_print("S");
        if (entry.long_mode)
            console_print("L");
        console_new_line();
    }
}

void idt_debug()
{
    Cpu *cpu = cpu_get_current();

    for (int i = 0; i < 256; i++)
    {
        IdtEntry entry = cpu->interrupt_descriptor_table[i];
        if (!entry.present)
            continue;
        console_print("[idt] entry #");
        console_print_u64(i, 10);
        console_print(" offset=0x");
        console_print_u64((unsigned long)entry.offset1 | ((unsigned long)entry.offset2 << 16) | ((unsigned long)entry.offset3 << 32), 16);
        console_print(" code_segment=");
        console_print_u64(entry.code_segment, 10);
        console_print(" type=0b");
        console_print_u64(entry.gate_type, 2);
        console_print(" level=");
        console_print_u64(entry.privilege_level, 10);
        if (entry.interrupt_stack_table_offset)
        {
            console_print(" ist=");
            console_print_u64(entry.interrupt_stack_table_offset, 10);
        }
        console_new_line();
    }
}

void root_program()
{
    gdt_debug();
    idt_debug();

    asm volatile("int3");
    // console_clear();
    // console_print("[cpu] root program start 2\n");

    // scheduler_execute(&test_program);
    // scheduler_execute(&test_program2);
    // scheduler_execute(&test_program3);

    // console_print("[cpu] root program end\n");
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

    // Read the memory map provided by multiboot, is contains information about what regions of RAM we can freely use
    // https://kokos.run/#WzAsIk11bHRpYm9vdC5wZGYiLDEzLFsxMywxNjIsMTMsMTYyXV0=
    Multiboot2InfoTagMemoryMap *memory_information = (Multiboot2InfoTagMemoryMap *)multiboot2_info_get(MULTIBOOT2_TYPE_MEMORY_MAP);
    if (!memory_information)
    {
        console_print("[physical memory] fatal: no memory information available\n");
        return;
    }

    // Find total amount of memory
    unsigned long usable_memory = 0;
    max_memory_address = 0;
    for (int i = 0; i * memory_information->entry_size < memory_information->base.size; i++)
    {
        Multiboot2InfoTagMemoryMapEntry *entry = &memory_information->entries[i];
        if (entry->type == 1)
        {
            usable_memory += entry->length;
            if (entry->address + entry->length > max_memory_address)
            {
                max_memory_address = entry->address + entry->length;
            }
        }

        console_print("[physical memory] memory at 0x");
        console_print_u64(entry->address, 16);
        console_print(" with length 0x");
        console_print_u64(entry->length, 16);
        console_print(" with type ");
        console_print_u32(entry->type, 10);
        console_print(" (0x");
        console_print_u64(max_memory_address, 16);
        console_print(")");
        console_new_line();
    }

    console_print("[physical memory] total usable ram = ");
    console_print_u64(usable_memory, 10);
    console_print(" from 0x0 ... 0x");
    console_print_u64(max_memory_address, 16);
    console_new_line();

    // Find first memory region that can fit allocation table
    // Each bit (8 per byte) in the allocation will determine if a memory region of 4096 bytes is taken or not
    unsigned long allocation_table_size = max_memory_address / 4096 / 8;

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

    memory_physical_initialize(allocation_table_start, max_memory_address);

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

    // Get CPU manufacturer https://kokos.run/#WzAsIkludGVsVm9sdW1lMkEucGRmIiwyOTIsWzI5MiwxOCwyOTIsMThdXQ==
    CpuIdResult cpu_name = cpu_id(0x0);
    console_print("[cpu] cpu manufacturer ");
    console_print_length(&cpu_name.ebx, 4);
    console_print_length(&cpu_name.edx, 4);
    console_print_length(&cpu_name.ecx, 4);
    console_new_line();

    console_print("[paging] initialize paging\n");

    // Initialize paging
    paging_initialize();

    console_print("[interrupt] disable pic\n");

    // Disable and remap pic before initializing the APCI in cpu_initialize
    apic_disable_pic();

    console_print("[cpu] initialize cpu context\n");

    cpu_initialize(root_program);

    // for (int i = 0; i < 3; i++)
    // {
    //     console_print("[paging] identity test: 0x");
    //     int *virt = (int *)0x000fe0000 + i;
    //     console_print_u64((unsigned long)virt, 16);
    //     console_print(" -> 0x");
    //     console_print_u64((unsigned long)paging_get_physical_address(virt), 16);
    //     console_print("\n");
    // }

    // for (int i = 0; i < 4; i++)
    // {
    //     console_print("[paging] allocate test: 0x");
    //     int *virt = paging_map(1000 * i + 123, PAGING_FLAG_WRITE | PAGING_FLAG_READ);
    //     console_print_u64((unsigned long)virt, 16);
    //     console_print(" -> 0x");
    //     console_print_u64((unsigned long)paging_get_physical_address(virt), 16);
    //     console_print("\n");

    //     // paging_debug();
    //     int a = *virt;

    //     *virt = i * 500;
    // }

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

    // https://kokos.run/#WzAsIkFDUEkucGRmIiwxNjFd
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

    Cpu *cpu = cpu_get_current();

    Apic *apic = cpu->local_apic;

    console_print("[apic] mapped at 0x");
    console_print_u64((unsigned long)apic, 16);
    console_new_line();

    unsigned char apic_id = (unsigned int)apic->id >> 24;
    unsigned char apic_version = apic->version & 0xFF;
    unsigned char apic_max_entries = (apic->version >> 16) & 0xFF;
    console_print("[apic] physical address: 0x");
    console_print_u64((unsigned long)paging_get_physical_address(&cpu->current_process->paging_context, apic), 16);
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
            ioapic = (IOApic *)paging_map_physical(&cpu->current_process->paging_context, current_ioapic->io_apic_address, sizeof(IOApic), PAGING_FLAG_READ | PAGING_FLAG_WRITE);

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

    console_clear();

    AcpiMadtEntry0LocalAPIC *current_processor = 0;
    while (current_processor = acpi_madt_iterate_type(madt, current_processor, ACPI_MADT_TYPE_LOCAL_APIC))
    {
        // console_print("[smp] starting processor ");
        // console_print_i32(current_processor->processor_id, 10);
        // console_print(" and wait");
        // console_new_line();

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

        // console_print("[smp] startup ok\n");
        // console_new_line();
    }

    cpu_startup_done = 1;

    // console_print("[smp] started all processors in a halted state\n");

    asm volatile("sti");

    while (1)
    {
        asm volatile("hlt");
    }

    // Enable APIC
    // apic->spurious_interrupt_vector = 0x1FF;

    // console_print("[test] create test interrupts\n");
    // console_print("[test] interrupt at 0x");
    // console_print_u64((unsigned long)interrupt_schedule, 16);
    // console_new_line();

    // idt_register_interrupt(0x22, interrupt_schedule, INTERRUPT_GATE_TYPE_INTERRUPT);
    // apic->timer_initial_count = 10000000;
    // apic->timer_divide_config = 0b1010;
    // apic->timer_vector = 0x22 | (1 << 17);

    idt_register_interrupt(0x24, interrupt_handle_timer, INTERRUPT_GATE_TYPE_INTERRUPT);
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

    // unsigned long flags;
    // asm volatile("pushfq; pop %0"
    //              : "=rm"(flags));
    // console_print("flags=0b");
    // console_print_u64(flags, 2);
    // console_new_line();
}
