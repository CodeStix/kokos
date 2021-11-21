#pragma once
#include "kokos/core.h"

enum idt_gate_type
{
    // Same as trap, but clears the interrupt flag on entry (disables hardware interrupts), this disables nested interrupts.
    IDT_GATE_TYPE_INTERRUPT = 0b1110,
    // A trap gate is recommended because this does not block other incoming hardware interrupts and allows nested interrupts.
    IDT_GATE_TYPE_TRAP = 0b1111,
};

enum idt_stack_type
{
    // Interrupt stack 0 does not exist, it means don't use an interrupt stack, just use the current stack.
    // When using an interrupt stack, the stack cannot be used by multiple interrupts at the same time because they would overwrite each others stacks.
    IDT_STACK_TYPE_CURRENT = 0,
    // Interrupt stack 1 is used for double faults, to make sure that a valid stack is available when this exception occures.
    IDT_STACK_TYPE_DOUBLE_FAULT = 1,
    // Stack 2 is used for scheduler interrupts and task switches
    // A seperate stack is used for the scheduler interrupt because, when a process must be switched, the stack pointer could be unique to that process's virtual address space.
    // Because the next process does not use the same virtual address space, problems would occur when switching virtual address spaces.
    // The scheduler stack would be mapped into every process' virtual address space.
    IDT_STACK_TYPE_SCHEDULER = 2,
    // See gdt.c to see how these are allocated
};

// This struct describes the format of the stack after an interrupt was fired. See AMD Volume 2 page 258.
// (Also needed by gcc to make the interrupt attribute work, https://gcc.gnu.org/onlinedocs/gcc/x86-Function-Attributes.html#x86-Function-Attributes)
struct idt_stack_frame
{
    unsigned long instruction_pointer;
    unsigned long code_segment;
    unsigned long rflags;
    unsigned long stack_pointer;
    unsigned long stack_segment;
};

// An entry in the IDT (interrupt descriptor table)
// https://wiki.osdev.org/IDT
struct idt_entry
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
} ATTRIBUTE_PACKED;

// This structure is used for the lidt instruction, which loads the interrupt descriptor table
struct idt_pointer
{
    unsigned short limit;
    struct idt_entry *address;
} ATTRIBUTE_PACKED;

// This function must be called before calling any other interrupt functions. This function fills the IDT only for the current cpu.
// memory_physical_initialize and cpu_initialize must be called first!!
void idt_initialize();

// Disables an interrupt vector
void idt_disable_interrupt(unsigned char vector);

// Enables an interrupt vector
void idt_enable_interrupt(unsigned char vector);

// Registers and enables an interrupt vector
void idt_register_interrupt(unsigned char vector, void *function_pointer, enum idt_gate_type interrupt_type, enum idt_stack_type ist);

// Prints the current idt to the console
void idt_debug();