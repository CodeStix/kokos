#include "../include/apic.h"
#include "../include/port.h"
#include "../include/cpu.h"

#define PIC_MASTER_COMMAND_PORT 0x20
#define PIC_MASTER_DATA_PORT 0x21
#define PIC_SLAVE_COMMAND_PORT 0xA0
#define PIC_SLAVE_DATA_PORT 0xA1

// static inline void wait()
// {
//     // Port 80 is unused by the computer and is used to simulate a delay, because it could take some time before the PIC answers
//     port_out8(0x80, 0);
// }

// TODO check if APIC is enabled in MSR

// Because we will use the newer APIC (advanced programmable interrupt controller) instead of the old PIC's
// we need to disable the old PIC's. Because PIC's can generate random interrupts (by electric noise ect), we must also offset these interrupts to an unused
// region of interrupt vectors, so it does not trigger random system interrupts (like exception interrupts at worst)

void apic_disable_pic()
{
    // Start master PIC initialization sequence, it now expects 3 bytes at port 0x21 to configure this PIC
    port_out8(PIC_MASTER_COMMAND_PORT, 0x11);
    cpu_wait_microsecond();

    // Byte 1: set master interrupt vectors to start at 247 (247 -> 254)
    port_out8(PIC_MASTER_DATA_PORT, 247);
    cpu_wait_microsecond();

    // Byte 2: tell master PIC that there is a slave PIC at IRQ2 pin
    port_out8(PIC_MASTER_DATA_PORT, 0b00000100);
    cpu_wait_microsecond();

    // Byte 3: PIC mode 1 (??)
    port_out8(PIC_MASTER_DATA_PORT, 0x01);
    cpu_wait_microsecond();

    // Start slave PIC initialization sequence, it now expects 3 bytes at port 0xA1 to configure this PIC
    port_out8(PIC_SLAVE_COMMAND_PORT, 0x11);
    cpu_wait_microsecond();

    // Byte 1: set slave interrupt vectors to start at 239 (239 -> 246)
    port_out8(PIC_SLAVE_DATA_PORT, 239);
    cpu_wait_microsecond();

    // Byte 2: tell slave PIC that it is a slave
    port_out8(PIC_SLAVE_DATA_PORT, 0b00000010);
    cpu_wait_microsecond();

    // Byte 3: PIC mode 1 (??)
    port_out8(PIC_SLAVE_DATA_PORT, 0x01);
    cpu_wait_microsecond();

    // Disable PICs (by masking all interrupts)
    port_out8(PIC_MASTER_DATA_PORT, 0b11111111);
    port_out8(PIC_SLAVE_DATA_PORT, 0b11111111);
}

int apic_check_supported()
{
    CpuIdResult result = cpu_id(0x1);
    // Test for bit 9 in the rdx register, which means that an apic is supported (AMD64 Volume 3 page 574)
    return result.edx & 0b100000000;
}

unsigned char apic_io_get_id(IOApic *apic)
{
    apic->register_select = APIC_IO_REGISTER_ID;
    return (apic->register_data >> 24) & 0b11111111;
}

unsigned char apic_io_get_version(IOApic *apic)
{
    apic->register_select = APIC_IO_REGISTER_VERSION;
    return apic->register_data & 0b11111111;
}

unsigned char apic_io_get_max_entries(IOApic *apic)
{
    // The amount of entries this IOAPIC supports is contained in the version register
    apic->register_select = APIC_IO_REGISTER_VERSION;
    return (apic->register_data >> 16) & 0b11111111;
}

unsigned long apic_io_get_entry(IOApic *apic, unsigned char index)
{
    // Each entry takes up 2 IOAPIC registers
    unsigned char reg = APIC_IO_REGISTER_ENTRY(index);
    // Read register 1
    apic->register_select = reg;
    unsigned long entry = apic->register_data;
    // Read register 2
    apic->register_select = reg + 1;
    // Combine their result
    return entry | ((unsigned long)apic->register_data << 32);
}

void apic_io_set_entry(IOApic *apic, unsigned char index, unsigned long entry)
{
    unsigned char reg = APIC_IO_REGISTER_ENTRY(index);
    // Set first register
    apic->register_select = reg;
    apic->register_data = entry & 0xFFFFFFFF;
    // Set second register
    apic->register_select = reg + 1;
    apic->register_data = entry >> 32;
}

void apic_io_register(IOApic *apic, unsigned char irq, IOApicEntry entry)
{
    apic_io_set_entry(apic, irq, *(unsigned long *)&entry);
}

#define MAX_IO_APIC 8

typedef struct
{
    unsigned int starting_irq; // Or GSI (Global System Interrupt)
    IOApic *address;
} IOApicInfo;
