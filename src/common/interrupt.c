#include "../include/interrupt.h"
#include "../include/console.h"

char *exception_messages[] = {
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
    "reserved",
    "x87 floating point exception",
    "alignment check",
    "machine check",
    "SIMD floating point",
    "reserved",
    "reserved",
    "reserved",
    "reserved",
    "reserved",
    "reserved",
    "reserved",
    "reserved",
    "hypervisor injection exception",
    "VMM communication exception",
    "security exception",
    "reserved",
};

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
            unsigned long error_code;
            asm volatile("mov %0, [rbp + 128]"
                         : "=r"(error_code)
                         :
                         :);

            console_print("page fault! process tried to access 0x");
            console_print_u64(fault_address, 16);
            console_print(", error code 0b");
            console_print_u64(error_code, 2);
            console_new_line();
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