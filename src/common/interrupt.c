#include "../include/interrupt.h"
#include "../include/console.h"

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

static char *breakpoint_register_names[] = {
    "r11",
    "r10",
    "r9",
    "r8",
    "rsi",
    "rdi",
    "rdx",
    "rcx",
    "rbx",
    "rax",
    "rip",
    "cs",
    "rflags",
    "rsp",
};

static void interrupt_handle_breakpoint(unsigned long *base_pointer)
{
    for (int i = 0; i < sizeof(breakpoint_register_names) / sizeof(breakpoint_register_names[0]); i++)
    {
        unsigned long value = base_pointer[i];
        console_print(breakpoint_register_names[i]);
        if (i == 12)
        {
            // Use binary for flags register
            console_print("=0b");
            console_print_u64(value, 2);
        }
        else
        {
            console_print("=0x");
            console_print_u64(value, 16);
        }
        console_print(" ");

        if ((i + 1) % 4 == 0)
        {
            console_new_line();
        }
    }
    console_new_line();
}

// When this function is called from interrupts.asm, the stack has the following format
// ss:rsp -> rflags -> cs -> rip -> rax -> rbx -> rcx -> rdx -> rdi -> rsi -> r8 -> r9 -> r10 -> r11 -> (rip) -> (rbp)
void interrupt_handle(int vector)
{
    if (vector < 0x20)
    {
        // Is cpu exception interrupt
        if (vector == 14)
        {
            // When a page fault happens, the address that was tried to be accessed, is in control register 2 (cr2). (AMD Volume 2 8.2.15)
            unsigned long fault_address;
            asm volatile("mov %0, cr2"
                         : "=r"(fault_address)
                         :
                         :);
            // When a page fault happens, the error code (which contains how the page fault happened), is pushed onto the stack by the processor.
            // Because all the registers were pushed onto the stack by the os (in interrupts.asm), we need to add 128 (8 bytes * 16 registers were pushed) bytes (the stack grows downwards)
            // to the stack base pointer to reference the value the processor pushed onto the stack. (AMD Volume 2 8.2.15)
            // Note: only interrupt vectors 10, 11, 12, 13, 14, 17 push an error code onto the stack
            unsigned long error_code;
            asm volatile("mov %0, [rbp + 8 * 12]"
                         : "=r"(error_code)
                         :
                         :);

            console_print("page fault! process tried to access 0x");
            console_print_u64(fault_address, 16);
            console_print(", error code 0b");
            console_print_u64(error_code, 2);
            console_new_line();
        }
        else if (vector == 3)
        {
            // Save the base pointer, because it would get overwritten when calling interrupt_handle_breakpoint
            unsigned long *base_pointer;
            asm volatile("mov %0, rbp"
                         : "=r"(base_pointer)
                         :
                         :);
            interrupt_handle_breakpoint(base_pointer + 2);
        }
        else
        {
            char *message = exception_messages[vector];
            console_print(message);
            console_new_line();
        }

        // Stop the processor when an exception occured, continue if a breakpoint was hit (interrupt 3)
        if (vector != 3)
        {
            asm volatile("hlt");
        }
    }
    else
    {
        // Is normal interrupt
        console_print("caught normal vector #");
        console_print_u32(vector, 10);
        console_new_line();
    }
}
