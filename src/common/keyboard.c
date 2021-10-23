#include "../include/keyboard.h"
#include "../include/console.h"
#include "../include/port.h"
#include "../include/cpu.h"

// This file contains the driver for the old Intel 8042 ps/2 controller, which is still emulated by most systems
// https://wiki.osdev.org/%228042%22_PS/2_Controller

#define KEYBOARD_DATA_REGISTER 0x60
#define KEYBOARD_COMMAND_REGISTER 0x64

// Blocks until data is available, this is determined by the input buffer status flag at bit 0 of the status register
static inline void keyboard_wait_can_read()
{
    while (!(port_in8(KEYBOARD_COMMAND_REGISTER) & 0b00000001))
    {
        asm volatile("pause");
    }
}

// Blocks until you can write to the controller's registers, this is determined by the input buffer status flag at bit 1 of the status register
static inline void keyboard_wait_can_write()
{
    while (port_in8(KEYBOARD_COMMAND_REGISTER) & 0b00000010)
    {
        asm volatile("pause");
    }
}

// Read all remaining data from the data port at 0x60 while there is data in the output buffer
static inline void keyboard_clear_buffer()
{
    while (port_in8(KEYBOARD_COMMAND_REGISTER) & 0b00000001)
    {
        port_in8(KEYBOARD_DATA_REGISTER);
    }
}

void keyboard_init()
{
    keyboard_clear_buffer();
    keyboard_wait_can_write();

    port_out8(KEYBOARD_COMMAND_REGISTER, 0xAA);
    cpu_wait();
    // keyboard_wait_can_read();

    unsigned char result = port_in8(KEYBOARD_DATA_REGISTER);
    if (result == 0x55)
    {
        console_print("keyboard test passed!\n");
    }
    else
    {
        console_print("keyboard test failed with result ");
        console_print_i32(result, 10);
        console_new_line();
    }

    // while (1)
    // {

    //     keyboard_wait_can_read();
    //     unsigned char result = port_in8(0x60);
    //     console_print("key ");
    //     console_print_u32(result, 10);
    //     console_new_line();
    // }
}