#pragma once
#include "kokos/core.h"

typedef enum IdtGateType
{
    // Same as trap, but clears the interrupt flag on entry (disables hardware interrupts), this disables nested interrupts.
    IDT_GATE_TYPE_INTERRUPT = 0b1110,
    // A trap gate is recommended because this does not block other incoming hardware interrupts and allows nested interrupts. Make sure to not use IST != 0 when using traps because stacks will be overwritten by nested interrupts.
    IDT_GATE_TYPE_TRAP = 0b1111,
} IdtGateType;

typedef enum IdtStack
{
    // Interrupt stack 0 does not exist, it means don't use an interrupt stack, just use the current stack.
    IDT_STACK_CURRENT = 0,
    // Interrupt stack 1 is used for double faults, to make sure that a valid stack is available when this exception occures
    IDT_STACK_DOUBLE_FAULT = 1,
    // Stack 2 is used for scheduler interrupts and task switches
    // A seperate stack is used for the scheduler interrupt because, when a process must be switched, the stack pointer could be unique to that process's virtual address space.
    // Because the next process does not use the same virtual address space, problems would occur when switching virtual address spaces.
    // The scheduler stack would be mapped into every process' virtual address space.
    IDT_STACK_SCHEDULER = 2,
    // See gdt.c to see how these are allocated
} IdtStack;

// This struct describes the format of the stack after an interrupt was fired. See AMD Volume 2 page 258.
// (Also needed by gcc to make the interrupt attribute work, https://gcc.gnu.org/onlinedocs/gcc/x86-Function-Attributes.html#x86-Function-Attributes)
typedef struct IdtFrame
{
    unsigned long instruction_pointer;
    unsigned long code_segment;
    unsigned long rflags;
    unsigned long stack_pointer;
    unsigned long stack_segment;
} IdtFrame;

// An entry in the IDT (interrupt descriptor table)
// https://wiki.osdev.org/IDT
typedef struct IdtEntry
{
    union
    {
        struct
        {
            unsigned short offset1;
            unsigned short code_segment;
            unsigned char interrupt_stack;

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
} ATTRIBUTE_PACKED IdtEntry;

// This structure is used for the lidt instruction, which loads the interrupt descriptor table
typedef struct
{
    unsigned short limit;
    IdtEntry *address;
} ATTRIBUTE_PACKED IdtPointer;

// This function must be called before calling any other interrupt functions. This function fills the IDT only for the current cpu.
// memory_physical_initialize and cpu_initialize must be called first!!
void idt_initialize();

// Disables an interrupt vector
void idt_disable_interrupt(unsigned char vector);

// Enables an interrupt vector
void idt_enable_interrupt(unsigned char vector);

// Registers and enables an interrupt vector
void idt_register_interrupt(unsigned char vector, void *function_pointer, IdtGateType interrupt_type, IdtStack ist);

// Prints the current idt to the console
void idt_debug();