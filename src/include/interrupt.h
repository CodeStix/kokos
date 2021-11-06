#pragma once

#define ATTRIBUTE_INTERRUPT __attribute__((interrupt))
#define INTERRUPT_TYPE_INTERRUPT 0b1110
#define INTERRUPT_TYPE_TRAP 0b1111
#define CODE_SEGMENT 8

typedef enum
{
    INTERRUPT_GATE_TYPE_INTERRUPT = 0b1110,
    INTERRUPT_GATE_TYPE_TRAP = 0b1111,
} InterruptGateType;

// Needed by gcc to make the interrupt attribute work
// https://gcc.gnu.org/onlinedocs/gcc/x86-Function-Attributes.html#x86-Function-Attributes
typedef struct
{
    unsigned long instruction_pointer;
    unsigned long code_segment;
    unsigned long rflags;
    unsigned long register_stack_pointer;
    unsigned long stack_segment;
} InterruptFrame;

// An entry in the IDT (interrupt descriptor table)
// https://wiki.osdev.org/IDT
typedef struct
{
    unsigned short offset1;
    unsigned short selector;
    unsigned char interrupt_stack_table;
    unsigned char type_attributes;
    unsigned short offset2;
    unsigned int offset3;
    unsigned int unused;
} __attribute__((packed)) InterruptDescriptor;

// This structure is used for the lidt instruction, which loads the interrupt descriptor table
typedef struct
{
    unsigned short limit;
    InterruptDescriptor *address;
} __attribute__((packed)) InterruptDescriptorPointer;

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
