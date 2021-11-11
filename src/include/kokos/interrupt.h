#pragma once
#include "kokos/core.h"

#define INTERRUPT_TYPE_INTERRUPT 0b1110
#define INTERRUPT_TYPE_TRAP 0b1111
#define CODE_SEGMENT 8

typedef enum
{
    INTERRUPT_GATE_TYPE_INTERRUPT = 0b1110,
    INTERRUPT_GATE_TYPE_TRAP = 0b1111,
} InterruptGateType;

// This struct describes the format of the stack after an interrupt was fired. See AMD Volume 2 page 258.
// (Also needed by gcc to make the interrupt attribute work, https://gcc.gnu.org/onlinedocs/gcc/x86-Function-Attributes.html#x86-Function-Attributes)
typedef struct
{
    unsigned long instruction_pointer;
    unsigned long code_segment;
    unsigned long rflags;
    unsigned long stack_pointer;
    unsigned long stack_segment;
} InterruptFrame;

// An entry in the IDT (interrupt descriptor table)
// https://wiki.osdev.org/IDT
typedef struct InterruptDescriptor
{
    union
    {
        struct
        {
            unsigned short offset1;
            unsigned short code_segment;
            unsigned char interrupt_stack_table_offset;

            unsigned char gate_type : 4;
            unsigned char unused1 : 1;
            unsigned char privilege_level : 2;
            unsigned char present : 1;

            unsigned short offset2;
            unsigned int offset3;
            unsigned int unused;
        };
        unsigned long as_long[2];
    };
} ATTRIBUTE_PACKED InterruptDescriptor;

// This structure is used for the lidt instruction, which loads the interrupt descriptor table
typedef struct
{
    unsigned short limit;
    InterruptDescriptor *address;
} ATTRIBUTE_PACKED InterruptDescriptorPointer;

// This function must be called before calling any other interrupt functions. This function fills the IDT only for the current cpu.
// memory_physical_initialize and cpu_initialize must be called first!!
void interrupt_initialize();

// This function will be called by interrupts.asm
void interrupt_handle(int vector);

// Disables an interrupt vector
void interrupt_disable(unsigned char vector);

// Enables an interrupt vector
void interrupt_enable(unsigned char vector);

// Registers and enables an interrupt vector
void interrupt_register(unsigned char vector, void *function_pointer, InterruptGateType interrupt_type);
