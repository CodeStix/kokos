#include "../include/keyboard.h"
#include "../include/console.h"
#include "../include/port.h"
#include "../include/cpu.h"
#include "../include/interrupt.h"
#include "../include/apic.h"
#include "../include/serial.h"
#include "../include/util.h"

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
    cpu_wait_microsecond();

    return port_in8(KEYBOARD_STATUS_REGISTER);
}

static inline unsigned char keyboard_read_data()
{
    // Wait for a very short delay
    cpu_wait_microsecond();

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
    cpu_wait_microsecond();

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
    cpu_wait_microsecond();

    // Keep looping until input buffer is empty
    while (port_in8(KEYBOARD_STATUS_REGISTER) & KEYBOARD_INPUT_BUFFER_FULL)
    {
        asm volatile("pause");
    }

    port_out8(KEYBOARD_DATA_REGISTER, data);
}

static int releasing = 0;
static int shift = 0;
static int caps_lock = 0;

ATTRIBUTE_INTERRUPT
void interrupt_handle_keyboard(InterruptFrame *frame)
{
    unsigned char input = port_in8(0x60);

    // console_print("[keyboard] got interrupt 0x");
    // console_print_i32(input, 16);
    // console_new_line();

    unsigned int x, y;
    console_get_cursor(&x, &y);
    console_set_cursor(70, 0);
    console_print("key=0x");
    console_print_i32(input, 16);
    console_print(" ");
    console_set_cursor(x, y);

    if (input == 0xF0)
    {
        // Next key will release
        releasing = 1;
    }
    else if (releasing)
    {
        if (input == 0x12 || input == 0x59)
        {
            // Shift
            shift = 0;
        }
        releasing = 0;
    }
    else
    {
        if (input == 0x12 || input == 0x59)
        {
            // Shift
            shift = 1;
        }
        else if (input == 0x63)
        {
            // Arrow keys
            //      0x63
            // 0x61 0x60 0x6A
            console_scroll(-1);
        }
        else if (input == 0x60)
        {
            console_scroll(1);
        }
        else if (input == 0x14)
        {
            // Caps lock
            caps_lock = !caps_lock;

            // Update keyboard led
            keyboard_write_data(0xED);
            // Wait for ACK
            keyboard_read_data();
            // Send new led statusses (CapsLock NumberLock ScrollLock)
            keyboard_write_data(caps_lock ? 0b100 : 0b000);
            // Wait for ACK
            keyboard_read_data();

            unsigned int x, y;
            console_get_cursor(&x, &y);
            console_set_cursor(70, 1);
            console_print(caps_lock ? "CAPSLOCK" : "        ");
            console_set_cursor(x, y);
        }
        else
        {
            const char **scancodes = shift || caps_lock ? scancodes_shift : scancodes_normal;
            if (scancodes[input])
            {
                if (input == 90)
                {
                    // Enter
                    if (shift)
                    {
                        console_print_char(' ');
                        console_clear();
                        console_set_cursor(0, 0);
                    }
                    else
                    {
                        console_print_char(' ');
                        console_new_line();
                    }
                }
                else if (input == 102)
                {
                    // Backspace
                    unsigned int x, y;
                    console_get_cursor(&x, &y);
                    if (x > 0)
                    {
                        console_print_char(' ');
                        console_set_cursor(x - 1, y);
                        console_print_char(' ');
                        console_set_cursor(x - 1, y);
                    }
                    else if (y > 0)
                    {
                        console_print_char(' ');
                        console_set_cursor(79, y - 1);
                    }
                }
                else
                {
                    console_print(scancodes[input]);
                }
            }
        }
    }

    // console_new_line();

    // char buffer[5];
    // convert_i32_string(buffer, input, 10);
    // for (char *c = buffer; *c; c++)
    // {
    //     serial_write(*c);
    // }
    // serial_write('\n');

    // while (port_in8(0x64) & 0b00000001)
    // {
    //     unsigned char input = port_in8(0x60);
    //     console_print(" 0x");
    //     console_print_i32(input, 16);
    // }

    apic_get()->end_of_interrupt = 0;
}

void keyboard_initialize()
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

    // Write new config byte to the ps/2 controller, enabling interrupts
    keyboard_write_command(0x60);
    keyboard_write_data(0b00000111);

    // Enable ps/2 ports
    keyboard_write_command(0xAE);
    keyboard_write_command(0xA8);

    // Reset ps/2 device
    keyboard_write_data(0xFF);
    // Should return ACK(0xFA)
    keyboard_read_data();
    // Should return 0xAA, meaning self test successful
    keyboard_read_data();

    // Tell keyboard to use scan code set 3 (easiest to implement)
    keyboard_write_data(0xF0);
    // Wait for ACK
    keyboard_read_data();
    // Send 3
    keyboard_write_data(3);
    // Wait for ACK
    keyboard_read_data();

    // https://web.archive.org/web/20170108131104/http://www.computer-engineering.org/ps2keyboard/scancodes3.html

    // console_print("setting led\n");
    // keyboard_write_data(0xED);
    // result = keyboard_read_data();
    // console_print("response 0: 0x");
    // console_print_i32(result, 16);
    // console_new_line();
    // keyboard_write_data(0b101);
    // result = keyboard_read_data();
    // console_print("response 1: 0x");
    // console_print_i32(result, 16);
    // console_new_line();

    console_print("registering interrupt\n");

    asm volatile("cli");
    interrupt_register(0x23, interrupt_handle_keyboard, INTERRUPT_GATE_TYPE_INTERRUPT);
    asm volatile("sti");

    IOApicEntry entry = {
        .vector = 0x23,
        .destination = (apic_get()->id >> 24) & 0b11111111,
    };

    apic_io_register(apic_io_get(), 1, entry);
    apic_io_register(apic_io_get(), 12, entry);

    console_print("keyboard configuration done\n");

    // while (1)
    // {

    //     keyboard_wait_can_read();
    //     unsigned char result = port_in8(0x60);
    //     console_print("key ");
    //     console_print_u32(result, 10);
    //     console_new_line();
    // }
}