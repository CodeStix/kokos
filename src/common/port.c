#include "../include/port.h"

// https://www.ibiblio.org/gferg/ldp/GCC-Inline-Assembly-HOWTO.html
// This file contains some C with inline assembly, meaning, writing x86 instructions directly in your C code instead of having a seperate
// file containing instructions. Because C does not have any way to generate the out and in instructions (without inline assembly), we must use inline assembly.
// Each inline assembly statement starts with 'asm', followed by 4 parameters seperated by colons.
// Example:
//
//  asm("mov %0, %1" : "=r"(output) : "r"("input") : /* nothing */);
//
// You don't have to specify all the parameters of the asm statement, for example:
//
//  asm("out %0, %1" : /* nothing here */ : "Nd"(port), "a"(value) : /* nothing here */)
//
// The asm parameters mean the following:
// 1. Instructions: the first parameter chooses which instruction(s) is generated, with %0, %1 ... %n meaning arguments.
// 2. Output arguments: specify the output registers used, in the format "constraints"(c_variable).
// 3. Input arguments: specify the input registers used, in the format "constraints"(c_variable).
// 4. Clobbered data: specify which registers are influenced by this instruction, so the compiler knows that these registers will contain garbage afterwards
//    and will not use them.

inline void port_out32(unsigned short port, unsigned int value)
{
    asm volatile("out %0, %1"
                 :
                 : "Nd"(port), "a"(value)
                 :);
}

inline void port_out16(unsigned short port, unsigned short value)
{

    asm volatile("out %0, %1"
                 :
                 : "Nd"(port), "a"(value)
                 :);
}

inline void port_out8(unsigned short port, unsigned char value)
{

    asm volatile("out %0, %1"
                 :
                 : "Nd"(port), "a"(value)
                 :);
}

inline unsigned int port_in32(unsigned short port)
{
    unsigned int value;
    asm volatile("in %0, %1"
                 : "=a"(value)
                 : "Nd"(port)
                 :);
    return value;
}

inline unsigned short port_in16(unsigned short port)
{
    unsigned short value;
    asm volatile("in %0, %1"
                 : "=a"(value)
                 : "Nd"(port)
                 :);
    return value;
}

inline unsigned char port_in8(unsigned short port)
{
    unsigned char value;
    asm volatile("in %0, %1"
                 : "=a"(value)
                 : "Nd"(port)
                 :);
    return value;
}
