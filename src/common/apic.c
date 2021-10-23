#include "../include/apic.h"
#include "../include/port.h"
#include "../include/cpu.h"

#define PIC_MASTER_COMMAND_PORT 0x20
#define PIC_MASTER_DATA_PORT 0x21
#define PIC_SLAVE_COMMAND_PORT 0xA0
#define PIC_SLAVE_DATA_PORT 0xA1

static inline void wait()
{
    // Port 80 is unused by the computer and is used to simulate a delay, because it could take some time before the PIC answers
    port_out8(0x80, 0);
}

// TODO check if APIC is enabled in MSR

// Because we will use the newer APIC (advanced programmable interrupt controller) instead of the old PICs
// we need to disable the old PIC. Because PIC's can generate random interrupts (by electric noise ect), we must also offset these interrupts to an unused
// region of interrupt vectors, so it does not trigger random system interrupts (worse: exception interrupts)

void apic_disable_pic()
{
    // Start master PIC initialization sequence, it now expects 3 bytes at port 0x21 to configure this PIC
    port_out8(PIC_MASTER_COMMAND_PORT, 0x11);
    wait();

    // Byte 1: set master interrupt vectors to start at 247 (247 -> 254)
    port_out8(PIC_MASTER_DATA_PORT, 247);
    wait();

    // Byte 2: tell master PIC that there is a slave PIC at IRQ2 pin
    port_out8(PIC_MASTER_DATA_PORT, 0b00000100);
    wait();

    // Byte 3: PIC mode 1 (??)
    port_out8(PIC_MASTER_DATA_PORT, 0x01);
    wait();

    // Start slave PIC initialization sequence, it now expects 3 bytes at port 0xA1 to configure this PIC
    port_out8(PIC_SLAVE_COMMAND_PORT, 0x11);
    wait();

    // Byte 1: set slave interrupt vectors to start at 239 (239 -> 246)
    port_out8(PIC_SLAVE_DATA_PORT, 239);
    wait();

    // Byte 2: tell slave PIC that it is a slave
    port_out8(PIC_SLAVE_DATA_PORT, 0b00000010);
    wait();

    // Byte 3: PIC mode 1 (??)
    port_out8(PIC_SLAVE_DATA_PORT, 0x01);
    wait();

    // Disable PICs (by masking all interrupts using)
    port_out8(PIC_MASTER_DATA_PORT, 0b11111111);
    port_out8(PIC_SLAVE_DATA_PORT, 0b11111111);
}

int apic_check_supported()
{
    CpuIdResult result = cpu_id(0x1);
    // Test for bit 9 in the rdx register, which means that an apic is supported (Amd64 Volume 3 page 574)
    return result.edx & 0b100000000;
}