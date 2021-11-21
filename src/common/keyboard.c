#include "kokos/keyboard.h"
#include "kokos/console.h"
#include "kokos/port.h"
#include "kokos/cpu.h"
#include "kokos/idt.h"
#include "kokos/apic.h"
#include "kokos/serial.h"
#include "kokos/util.h"

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
static int extended = 0;

ATTRIBUTE_INTERRUPT
void interrupt_handle_keyboard(struct idt_stack_frame *frame)
{
    unsigned char input = port_in8(0x60);

    // console_print("[keyboard] got interrupt 0x");
    // console_print_i32(input, 16);
    // console_new_line();

    if (input == 0xF0)
    {
        // Next key will release
        releasing = 1;
    }
    else if (input == 0xE0 || input == 0xE1 || input == 0xE2)
    {
        // Next key is extended
        extended = 1;
    }
    else
    {
        if (releasing)
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
            unsigned int x, y;
            console_get_cursor(&x, &y);
            console_set_cursor(70, 0);
            console_print("key=0x");
            console_print_u32(input, 16);
            console_set_cursor(x, y);

            if (input == 0x12 || input == 0x59)
            {
                // Shift
                shift = 1;
            }
            else if (extended && input == 0x75)
            {
                // Up arrow
                console_scroll(-1);
            }
            else if (extended && input == 0x72)
            {
                // Down arrow
                console_scroll(1);
            }
            else if (input == 0x58)
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
                    if (input == 0x5a)
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
                    else if (input == 0x66)
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
        extended = 0;
    }

    struct cpu *cpu = cpu_get_current();
    cpu->local_apic_physical->end_of_interrupt = 0;
}

extern struct ioapic *ioapic;

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

    // Tell keyboard to use scan code set 2
    keyboard_write_data(0xF0);
    // Wait for ACK
    keyboard_read_data();
    // Send 2
    keyboard_write_data(2);
    // Wait for ACK
    keyboard_read_data();

    console_print("registering interrupt\n");

    asm volatile("cli");
    idt_register_interrupt(0x23, interrupt_handle_keyboard, IDT_GATE_TYPE_TRAP, 0);
    asm volatile("sti");

    struct cpu *cpu = cpu_get_current();

    struct ioapic_entry entry = {
        .vector = 0x23,
        .destination = (cpu->local_apic_physical->id >> 24) & 0b11111111,
    };

    apic_io_register(ioapic, 1, entry);
    // apic_io_register(apic_io_get(), 12, entry);

    console_print("keyboard configuration done\n");
}