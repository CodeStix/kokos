#include "../include/keyboard.h"
#include "../include/console.h"

// This file contains the driver for the old Intel 8042 ps/2 controller, which is still emulated by most systems
// https://wiki.osdev.org/%228042%22_PS/2_Controller

extern void port_out32(unsigned short port, unsigned int value);
extern unsigned int port_in32(unsigned short port);
extern void port_out8(unsigned short port, unsigned char value);
extern unsigned char port_in8(unsigned short port);

#define KEYBOARD_DATA_REGISTER 0x60
#define KEYBOARD_COMMAND_REGISTER 0x64

// Blocks until data is available, this is determined by the input buffer status flag at bit 0 of the status register
void keyboard_wait_can_read()
{
    while (!(port_in8(KEYBOARD_COMMAND_REGISTER) & 0x1))
    {
    }
}

// Blocks until you can write to the controller's registers, this is determined by the input buffer status flag at bit 1 of the status register
void keyboard_wait_can_write()
{
    while (port_in8(KEYBOARD_COMMAND_REGISTER) & 0x2)
    {
    }
}

// Read all remaining data from the data port at 0x60 while there is data in the output buffer
void keyboard_clear_buffer()
{
    while (port_in8(KEYBOARD_COMMAND_REGISTER) & 0x1)
    {
        port_in8(KEYBOARD_DATA_REGISTER);
    }
}

void keyboard_init()
{
    console_print("enabling keyboard\n");

    keyboard_wait_can_write();

    // 0x64 is the ps2 command port
    // Send 'Disable first PS/2 port'
    port_out8(KEYBOARD_COMMAND_REGISTER, 0xAD);

    keyboard_wait_can_write();

    // Send 'Disable second PS/2 port', will be ignored if it only has one
    port_out8(KEYBOARD_COMMAND_REGISTER, 0xA7);

    // Remove previous data that may have been in the controller's buffer
    keyboard_clear_buffer();

    // Send 'Read byte 0 from internal RAM' command, this will contain the controller's configuration byte
    port_out8(KEYBOARD_COMMAND_REGISTER, 0x20);

    // Wait for response
    keyboard_wait_can_read();

    // Read from data register, which will now contain the configuration byte
    unsigned char config = port_in8(KEYBOARD_DATA_REGISTER);

    // Disable interrupts and port translation
    config = config & ~0b01000011;

    // Wait for the controller to be ready to receive data
    keyboard_wait_can_write();

    // Write the new config to the controller using the 'Write next byte to "byte 0" of internal RAM' command
    port_out8(KEYBOARD_COMMAND_REGISTER, 0x60);

    keyboard_wait_can_write();

    // Place the new config in the data port
    port_out8(KEYBOARD_DATA_REGISTER, config);

    keyboard_wait_can_write();

    // Send 'Test controller' command
    port_out8(KEYBOARD_COMMAND_REGISTER, 0xAA);

    keyboard_wait_can_read();

    unsigned char controller_test_result = port_in8(KEYBOARD_DATA_REGISTER);
    if (controller_test_result != 0x55)
    {
        console_print("ps2 controller self test failed!\n");
        return;
    }

    keyboard_wait_can_write();

    // Test ps2 port 1
    port_out8(KEYBOARD_COMMAND_REGISTER, 0xAB);

    keyboard_wait_can_read();

    unsigned char port1_test_result = port_in8(KEYBOARD_DATA_REGISTER);
    if (port1_test_result != 0)
    {
        console_print("warning: port 1 test failed!\n");
    }

    // Check if the 'disable second ps2 port' bit is set, if not, we know that this controller doesn't support a second ps2 port because it didn't disable it at step 1
    if (config & 0b00010000)
    {
        // TODO check if secondary port is supported by enabling and disabling the secondary port

        keyboard_wait_can_write();

        // Test ps2 port 2
        port_out8(KEYBOARD_COMMAND_REGISTER, 0xA9);

        keyboard_wait_can_read();

        unsigned char port2_test_result = port_in8(KEYBOARD_DATA_REGISTER);
        if (port2_test_result != 0)
        {
            console_print("warning: port 2 test failed!\n");
        }
    }

    console_print("ps2 controller initialized\n");
}