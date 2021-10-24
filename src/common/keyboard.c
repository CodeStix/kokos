#include "../include/keyboard.h"
#include "../include/console.h"
#include "../include/port.h"
#include "../include/cpu.h"
#include "../include/interrupt.h"
#include "../include/apic.h"

// This file contains the driver for the old Intel 8042 ps/2 controller, which is still emulated by most systems
// https://wiki.osdev.org/%228042%22_PS/2_Controller

#define KEYBOARD_DATA_REGISTER 0x60
#define KEYBOARD_COMMAND_REGISTER 0x64
#define KEYBOARD_STATUS_REGISTER 0x64

#define KEYBOARD_OUTPUT_BUFFER_FULL 0b00000001
#define KEYBOARD_INPUT_BUFFER_FULL 0b00000010

static inline unsigned char keyboard_read_status()
{
    // Wait for a very short delay
    cpu_wait();

    return port_in8(KEYBOARD_STATUS_REGISTER);
}

static inline unsigned char keyboard_read_data()
{
    // Wait for a very short delay
    cpu_wait();

    // Keep looping until the output buffer is full
    while (!(port_in8(KEYBOARD_STATUS_REGISTER) & KEYBOARD_OUTPUT_BUFFER_FULL))
    {
        asm volatile("pause");
    }

    // Then read from its data register
    return port_in8(KEYBOARD_DATA_REGISTER);
}

static inline void keyboard_write_command(unsigned char command)
{
    // Wait for a very short delay
    cpu_wait();

    // Keep looping until input buffer is empty
    while (port_in8(KEYBOARD_STATUS_REGISTER) & KEYBOARD_INPUT_BUFFER_FULL)
    {
        asm volatile("pause");
    }

    port_out8(KEYBOARD_COMMAND_REGISTER, command);
}

static inline void keyboard_write_data(unsigned char data)
{
    // Wait for a very short delay
    cpu_wait();

    // Keep looping until input buffer is empty
    while (port_in8(KEYBOARD_STATUS_REGISTER) & KEYBOARD_INPUT_BUFFER_FULL)
    {
        asm volatile("pause");
    }

    port_out8(KEYBOARD_DATA_REGISTER, data);
}

ATTRIBUTE_INTERRUPT
void interrupt_handle_keyboard(InterruptFrame *frame)
{
    console_print("[keyboard] got interrupt 0x");
    unsigned char input = port_in8(0x60);
    console_print_i32(input, 16);
    // while (port_in8(0x64) & 0b00000001)
    // {
    //     unsigned char input = port_in8(0x60);
    //     console_print(" 0x");
    //     console_print_i32(input, 16);
    // }
    console_new_line();

    apic_get()->end_of_interrupt = 0;
}

void keyboard_init()
{
    console_print("testing keyboard\n");

    keyboard_write_command(0xAA);

    unsigned char result = keyboard_read_data();
    if (result != 0x55)
    {
        console_print("keyboard test FAILED!\n");
        return;
    }

    console_print("keyboard test OK!\n");

    console_print("testing keyboard end\n");

    // asm volatile("cli");
    interrupt_register(0x23, interrupt_handle_keyboard, INTERRUPT_GATE_TYPE_INTERRUPT);
    // asm volatile("sti");

    // while (1)
    // {

    //     keyboard_wait_can_read();
    //     unsigned char result = port_in8(0x60);
    //     console_print("key ");
    //     console_print_u32(result, 10);
    //     console_new_line();
    // }
}