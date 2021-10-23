#include "../include/interrupt.h"
#include "../include/console.h"
#include "../include/memory_physical.h"

static char *exception_messages[] = {
    "divide by zero",
    "debug",
    "external non maskable interrupt",
    "breakpoint",
    "overflow",
    "bound range",
    "invalid opcode",
    "device not available",
    "double fault",
    "coprocessor segment overrun",
    "invalid tss",
    "segment not present",
    "stack",
    "general protection",
    "page fault",
    "reserved15",
    "x87 floating point exception",
    "alignment check",
    "machine check",
    "SIMD floating point",
    "reserved20",
    "reserved21",
    "reserved22",
    "reserved23",
    "reserved24",
    "reserved25",
    "reserved26",
    "reserved27",
    "hypervisor injection exception",
    "VMM communication exception",
    "security exception",
    "reserved",
};

static InterruptDescriptor *interrupt_descriptor_table;
static unsigned short code_segment;

// https://gcc.gnu.org/onlinedocs/gcc/x86-Function-Attributes.html#x86-Function-Attributes
ATTRIBUTE_INTERRUPT
static void interrupt_handle_divide_by_zero(InterruptFrame *frame)
{
    console_print("divide by zero!\n");
    console_new_line();

    asm volatile("hlt");
}

ATTRIBUTE_INTERRUPT
static void interrupt_handle_page_fault(InterruptFrame *frame, unsigned long error_code)
{
    // When a page fault happens, the address that was tried to be accessed, is in control register 2 (cr2). (AMD Volume 2 8.2.15)
    unsigned long fault_address;
    asm volatile("mov %0, cr2"
                 : "=r"(fault_address)
                 :
                 :);
    // When a page fault happens, the error code (which contains how the page fault happened), is pushed onto the stack by the processor.
    // Note: only interrupt vectors 10, 11, 12, 13, 14, 17 push an error code onto the stack
    console_print("page fault! process tried to access 0x");
    console_print_u64(fault_address, 16);
    console_print(", error code 0b");
    console_print_u64(error_code, 2);
    console_new_line();

    asm volatile("hlt");
}

ATTRIBUTE_INTERRUPT
static void interrupt_handle_breakpoint(InterruptFrame *frame)
{
    console_print("hit breakpoint!\n");
    console_new_line();
}

void interrupt_initialize()
{
    // The interrupt descriptor table just fits in a single page (16 bytes interrupt descriptor * 256 entries)
    interrupt_descriptor_table = (InterruptDescriptor *)memory_physical_allocate();
    code_segment = 8; // TODO

    if (!interrupt_descriptor_table)
    {
        console_print("fatal: could not initialize interrupt descriptor table");
        asm volatile("hlt");
    }

    // Fill with zeroes
    for (int i = 0; i < 4096; i += sizeof(unsigned long))
    {
        ((unsigned long *)interrupt_descriptor_table)[i] = 0ull;
    }

    InterruptDescriptorPointer pointer = {
        .address = interrupt_descriptor_table,
        .limit = 256 * 16 - 1,
    };

    // Load interrupt descriptor table
    asm volatile("lidt [%0]"
                 :
                 : "m"(pointer)
                 :);

    interrupt_register(3, interrupt_handle_breakpoint, INTERRUPT_GATE_TYPE_TRAP);
    interrupt_register(14, interrupt_handle_page_fault, INTERRUPT_GATE_TYPE_TRAP);
    interrupt_register(0, interrupt_handle_divide_by_zero, INTERRUPT_GATE_TYPE_TRAP);
}

void interrupt_disable(unsigned char vector)
{
    interrupt_descriptor_table[vector].type_attributes &= ~(1 << 7);
}

void interrupt_enable(unsigned char vector)
{
    interrupt_descriptor_table[vector].type_attributes |= (1 << 7);
}

void interrupt_register(unsigned char vector, void *function_pointer, InterruptGateType interrupt_type)
{
    if (interrupt_descriptor_table[vector].type_attributes & (1 << 7))
    {
        console_print("warning: overwriting interrupt vector #");
        console_print_i32(vector, 10);
        console_new_line();
    }

    InterruptDescriptor *descriptor = &interrupt_descriptor_table[vector];
    descriptor->offset1 = (unsigned long)function_pointer & 0xFFFF;
    descriptor->selector = code_segment;
    descriptor->interrupt_stack_table = 0;                           // Unused
    descriptor->type_attributes = (interrupt_type & 0xF) | (1 << 7); // 1 << 7 enables interrupt
    descriptor->offset2 = ((unsigned long)function_pointer >> 16) & 0xFFFF;
    descriptor->offset3 = ((unsigned long)function_pointer >> 32) & 0xFFFFFFFF;
}
