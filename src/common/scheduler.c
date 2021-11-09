#include "scheduler.h"
#include "cpu.h"
#include "interrupt.h"

// Defined in src/x86_64/schedule.asm, this assembly code calls scheduler_handle_interrupt below
extern void(scheduler_interrupt)();

void scheduler_handle_interrupt(SchedulerInterruptFrame *stack)
{
    Cpu *current_cpu = cpu_get_current();

    // TODO preserve xmm/mmx registers

    console_print("[schedule] interrupt fired on cpu 0x");
    console_print_u64(current_cpu->id, 16);
    console_print(" when at ");
    console_print_u64(stack->base.code_segment, 16);
    console_print(":0x");
    console_print_u64((unsigned long)stack->base.instruction_pointer, 16);
    console_new_line();

    current_cpu->local_apic->end_of_interrupt = 0;
}

void scheduler_initialize()
{
    Cpu *cpu = cpu_get_current();

    interrupt_register(0x22, scheduler_interrupt, INTERRUPT_GATE_TYPE_INTERRUPT);
    cpu->local_apic->spurious_interrupt_vector = 0x1FF; // TODO move to cpu_initialize
    cpu->local_apic->timer_initial_count = 10000000;
    cpu->local_apic->timer_divide_config = 0b1010;
    cpu->local_apic->timer_vector = 0x22 | (1 << 17);
}