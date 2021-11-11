#include "kokos/idt.h"
#include "kokos/console.h"
#include "kokos/memory_physical.h"
#include "kokos/cpu.h"
#include "kokos/util.h"

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

// https://gcc.gnu.org/onlinedocs/gcc/x86-Function-Attributes.html#x86-Function-Attributes
ATTRIBUTE_INTERRUPT
static void idt_handle_divide_by_zero(IdtFrame *frame)
{
    console_print("interrupt: divide by zero!\n");

    asm volatile("cli\n"
                 "hlt");
}

ATTRIBUTE_INTERRUPT
static void idt_handle_debug(IdtFrame *frame)
{
    console_print("interrupt: debug!\n");
}

ATTRIBUTE_INTERRUPT
static void idt_handle_non_maskable_interrupt(IdtFrame *frame)
{
    console_print("interrupt: non maskable!\n");

    asm volatile("cli\n"
                 "hlt");
}

ATTRIBUTE_INTERRUPT
static void idt_handle_breakpoint(IdtFrame *frame)
{
    console_print("interrupt: breakpoint!\n");
}

ATTRIBUTE_INTERRUPT
static void idt_handle_overflow(IdtFrame *frame)
{
    console_print("interrupt: overflow!\n");

    asm volatile("cli\n"
                 "hlt");
}

ATTRIBUTE_INTERRUPT
static void idt_handle_bound_range(IdtFrame *frame)
{
    console_print("interrupt: bound range!\n");

    asm volatile("cli\n"
                 "hlt");
}

ATTRIBUTE_INTERRUPT
static void idt_handle_invalid_opcode(IdtFrame *frame)
{
    console_print("interrupt: invalid opcode! at 0x\n");
    console_print_u64(frame->instruction_pointer, 16);
    console_new_line();

    asm volatile("cli\n"
                 "hlt");
}

ATTRIBUTE_INTERRUPT
static void idt_handle_device_not_available(IdtFrame *frame)
{
    console_print("interrupt: device not available!\n");

    asm volatile("cli\n"
                 "hlt");
}

ATTRIBUTE_INTERRUPT
static void idt_handle_invalid_double_fault(IdtFrame *frame, unsigned long error_code)
{
    console_print("interrupt: double fault!\n");

    asm volatile("cli\n"
                 "hlt");
}

ATTRIBUTE_INTERRUPT
static void idt_handle_segment_overrun(IdtFrame *frame)
{
    console_print("interrupt: segment overrun!\n");

    asm volatile("cli\n"
                 "hlt");
}

ATTRIBUTE_INTERRUPT
static void idt_handle_invalid_tss(IdtFrame *frame, unsigned long error_code)
{
    console_print("interrupt: invalid tss!\n");

    asm volatile("cli\n"
                 "hlt");
}

ATTRIBUTE_INTERRUPT
static void idt_handle_segment_not_present(IdtFrame *frame, unsigned long error_code)
{
    console_print("interrupt: segment not present!\n");

    asm volatile("cli\n"
                 "hlt");
}

ATTRIBUTE_INTERRUPT
static void idt_handle_stack(IdtFrame *frame, unsigned long error_code)
{
    console_print("interrupt: stack!\n");

    asm volatile("cli\n"
                 "hlt");
}

ATTRIBUTE_INTERRUPT
static void idt_handle_general_protection(IdtFrame *frame, unsigned long error_code)
{
    console_print("interrupt: general protection!\n");

    asm volatile("cli\n"
                 "hlt");
}

ATTRIBUTE_INTERRUPT
static void idt_handle_page_fault(IdtFrame *frame, unsigned long error_code)
{
    // When a page fault happens, the address that was tried to be accessed, is in control register 2 (cr2). (AMD Volume 2 8.2.15)
    unsigned long fault_address;
    asm volatile("mov %0, cr2"
                 : "=r"(fault_address)
                 :
                 :);
    // When a page fault happens, the error code (which contains how the page fault happened), is pushed onto the stack by the processor.
    // Note: only interrupt vectors 8, 10, 11, 12, 13, 14, 17 push an error code onto the stack
    console_print("interrupt: page fault! process at 0x");
    console_print_u64(frame->instruction_pointer, 16);
    console_print(" tried to access 0x");
    console_print_u64(fault_address, 16);
    console_print(", error code 0b");
    console_print_u64(error_code, 2);
    console_new_line();

    asm volatile("cli\n"
                 "hlt");
}

ATTRIBUTE_INTERRUPT
static void idt_handle_float_exception(IdtFrame *frame)
{
    console_print("interrupt: floating-point exception!\n");

    asm volatile("cli\n"
                 "hlt");
}

ATTRIBUTE_INTERRUPT
static void idt_handle_alignment_check(IdtFrame *frame, unsigned long error_code)
{
    console_print("interrupt: alignment check!\n");

    asm volatile("cli\n"
                 "hlt");
}

ATTRIBUTE_INTERRUPT
static void idt_handle_machine_check(IdtFrame *frame)
{
    console_print("interrupt: machine check!\n");

    asm volatile("cli\n"
                 "hlt");
}

ATTRIBUTE_INTERRUPT
static void idt_handle_simd_float_exception(IdtFrame *frame)
{
    console_print("interrupt: simd floating-point exception!\n");

    asm volatile("cli\n"
                 "hlt");
}

ATTRIBUTE_INTERRUPT
static void idt_handle_spurious(IdtFrame *frame)
{
    // console_print("interrupt: spurious\n");
    // console_new_line();
}

void idt_initialize()
{
    // Disable interrupts
    // asm volatile("cli");

    // The interrupt descriptor table just fits in a single page (16 bytes interrupt descriptor * 256 entries)
    IdtEntry *interrupt_descriptor_table = (IdtEntry *)memory_physical_allocate();
    Cpu *cpu = cpu_get_current();
    cpu->interrupt_descriptor_table = interrupt_descriptor_table;

    // console_print("[interrupt] create interrupt_descriptor_table at 0x");
    // console_print_u64(interrupt_descriptor_table, 16);
    // console_print(" for cpu ");
    // console_print_u64(cpu->id, 10);
    // console_print(" at 0x");
    // console_print_u64(cpu, 16);
    // console_new_line();

    if (!interrupt_descriptor_table)
    {
        console_print("fatal: could not initialize interrupt descriptor table");
        asm volatile("hlt");
    }

    // Fill with zeroes
    for (int i = 0; i < 4096 / sizeof(unsigned long); i++)
    {
        ((unsigned long *)interrupt_descriptor_table)[i] = 0ull;
    }

    IdtPointer pointer = {
        .address = interrupt_descriptor_table,
        .limit = 256 * 16 - 1,
    };

    // Load interrupt descriptor table
    asm volatile("lidt [%0]"
                 :
                 : "m"(pointer)
                 :);

    idt_register_interrupt(0, idt_handle_divide_by_zero, INTERRUPT_GATE_TYPE_TRAP);
    idt_register_interrupt(1, idt_handle_debug, INTERRUPT_GATE_TYPE_TRAP);
    idt_register_interrupt(2, idt_handle_non_maskable_interrupt, INTERRUPT_GATE_TYPE_INTERRUPT); // This is recommended by Intel (Intel Volume 1 6.7.1)
    idt_register_interrupt(3, idt_handle_breakpoint, INTERRUPT_GATE_TYPE_TRAP);
    idt_register_interrupt(4, idt_handle_overflow, INTERRUPT_GATE_TYPE_TRAP);
    idt_register_interrupt(5, idt_handle_bound_range, INTERRUPT_GATE_TYPE_TRAP);
    idt_register_interrupt(6, idt_handle_invalid_opcode, INTERRUPT_GATE_TYPE_TRAP);
    idt_register_interrupt(7, idt_handle_device_not_available, INTERRUPT_GATE_TYPE_TRAP);
    idt_register_interrupt(8, idt_handle_invalid_double_fault, INTERRUPT_GATE_TYPE_TRAP);
    idt_register_interrupt(9, idt_handle_segment_overrun, INTERRUPT_GATE_TYPE_TRAP);
    idt_register_interrupt(10, idt_handle_invalid_tss, INTERRUPT_GATE_TYPE_TRAP);
    idt_register_interrupt(11, idt_handle_segment_not_present, INTERRUPT_GATE_TYPE_TRAP);
    idt_register_interrupt(12, idt_handle_stack, INTERRUPT_GATE_TYPE_TRAP);
    idt_register_interrupt(13, idt_handle_general_protection, INTERRUPT_GATE_TYPE_TRAP);
    idt_register_interrupt(14, idt_handle_page_fault, INTERRUPT_GATE_TYPE_TRAP);
    idt_register_interrupt(16, idt_handle_float_exception, INTERRUPT_GATE_TYPE_TRAP);
    idt_register_interrupt(17, idt_handle_alignment_check, INTERRUPT_GATE_TYPE_TRAP);
    idt_register_interrupt(18, idt_handle_machine_check, INTERRUPT_GATE_TYPE_TRAP);
    idt_register_interrupt(19, idt_handle_simd_float_exception, INTERRUPT_GATE_TYPE_TRAP);

    // The last 17 interrupts handle spurious interrupts (16 from the PIC and 1 from the APIC)
    for (int i = 0xff - 17; i <= 0xff; i++)
    {
        idt_register_interrupt(i, idt_handle_spurious, INTERRUPT_GATE_TYPE_TRAP);
    }

    // Enable interrupts
    // asm volatile("sti");
}

void idt_disable_interrupt(unsigned char vector)
{
    Cpu *cpu = cpu_get_current();
    cpu->interrupt_descriptor_table[vector].present = 0;
}

void idt_enable_interrupt(unsigned char vector)
{
    Cpu *cpu = cpu_get_current();
    cpu->interrupt_descriptor_table[vector].present = 1;
}

void idt_register_interrupt(unsigned char vector, void *function_pointer, IdtGateType interrupt_type)
{
    Cpu *cpu = cpu_get_current();

    if (cpu->interrupt_descriptor_table[vector].present)
    {
        console_print("warning: overwriting interrupt vector #");
        console_print_i32(vector, 10);
        console_new_line();
    }

    IdtEntry *descriptor = &cpu->interrupt_descriptor_table[vector];
    descriptor->offset1 = (unsigned long)function_pointer & 0xFFFF;
    descriptor->code_segment = CODE_SEGMENT;
    descriptor->interrupt_stack_table_offset = 0; // Unused
    descriptor->gate_type = interrupt_type & 0xF;
    descriptor->present = 1;
    descriptor->offset2 = ((unsigned long)function_pointer >> 16) & 0xFFFF;
    descriptor->offset3 = ((unsigned long)function_pointer >> 32) & 0xFFFFFFFF;
}

void idt_debug()
{
    Cpu *cpu = cpu_get_current();

    for (int i = 0; i < 256; i++)
    {
        IdtEntry entry = cpu->interrupt_descriptor_table[i];
        if (!entry.present)
            continue;
        console_print("[idt] entry #");
        console_print_u64(i, 10);
        console_print(" offset=0x");
        console_print_u64((unsigned long)entry.offset1 | ((unsigned long)entry.offset2 << 16) | ((unsigned long)entry.offset3 << 32), 16);
        console_print(" code_segment=");
        console_print_u64(entry.code_segment, 10);
        console_print(" type=0b");
        console_print_u64(entry.gate_type, 2);
        console_print(" level=");
        console_print_u64(entry.privilege_level, 10);
        if (entry.interrupt_stack_table_offset)
        {
            console_print(" ist=");
            console_print_u64(entry.interrupt_stack_table_offset, 10);
        }
        console_new_line();
    }
}