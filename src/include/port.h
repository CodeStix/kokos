#pragma once

void port_out32(unsigned short port, unsigned int value);
unsigned int port_in32(unsigned short port);
void port_out16(unsigned short port, unsigned short value);
unsigned short port_in16(unsigned short port);
void port_out8(unsigned short port, unsigned char value);
unsigned char port_in8(unsigned short port);
