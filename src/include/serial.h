#pragma once

// https://wiki.osdev.org/Serial_ports

#define SERIAL_PORT 0x3F8
#define SERIAL_REGISTER0_PORT SERIAL_PORT
#define SERIAL_REGISTER1_PORT SERIAL_PORT + 1
#define SERIAL_REGISTER2_PORT SERIAL_PORT + 2
#define SERIAL_REGISTER3_PORT SERIAL_PORT + 3
#define SERIAL_REGISTER4_PORT SERIAL_PORT + 4
#define SERIAL_REGISTER5_PORT SERIAL_PORT + 5
#define SERIAL_REGISTER6_PORT SERIAL_PORT + 6
#define SERIAL_REGISTER7_PORT SERIAL_PORT + 7

void serial_initialize();
int serial_available();
char serial_read();
void serial_write(char c);
void serial_print_u32(unsigned int num, unsigned int base);
void serial_print(const char *str);