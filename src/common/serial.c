#include "../include/port.h"
#include "../include/serial.h"
#include "../include/console.h"
#include "../include/cpu.h"

int serial_available()
{
    return port_in8(SERIAL_REGISTER5_PORT) & 1;
}

char serial_read()
{
    // Wait until data available
    while (!serial_available())
    {
        asm volatile("pause");
    }

    return port_in8(SERIAL_REGISTER0_PORT);
}

void serial_write(char c)
{
    // Wait until the transmission buffer is empty
    while (!(port_in8(SERIAL_REGISTER5_PORT) & 0b00100000))
    {
        asm volatile("pause");
    }

    port_out8(SERIAL_REGISTER0_PORT, (unsigned char)c);
}

void serial_initialize()
{
    console_print("[serial] enabling serial\n");

    // Disable serial interrupts
    port_out8(SERIAL_REGISTER1_PORT, 0);

    // Enable BAUD rate setup, the first 2 registers will now be used to set the baud rate
    port_out8(SERIAL_REGISTER3_PORT, 0b10000000);

    // Set the BAUD rate (115200 / 3 = 38400)
    port_out8(SERIAL_REGISTER0_PORT, 3);
    port_out8(SERIAL_REGISTER1_PORT, 0);

    // Use 8 data bits (0b11), 1 stop bit (0b100) and no parity
    port_out8(SERIAL_REGISTER3_PORT, 0b11 | 0b100);

    console_print("[serial] sending test data\n");

    for (int i = 0; i < 25; i++)
    {
        serial_write('A' + i);
    }

    console_print("[serial] initialized\n");
}