#include "scheduler.h"
#include "cpu.h"
#include "interrupt.h"

// Defined in src/x86_64/schedule.asm, this assembly code calls scheduler_handle_interrupt below
extern void(scheduler_interrupt)();

void scheduler_handle_interrupt(SchedulerInterruptFrame *stack)
{
    Cpu *current_cpu = cpu_get_current();

    // console_print("[schedule] interrupt fired on cpu 0x");
    // console_print_u64(current_cpu->id, 16);
    // console_print(" when at ");
    // console_print_u64(stack->base.code_segment, 16);
    // console_print(":0x");
    // console_print_u64((unsigned long)stack->base.instruction_pointer, 16);
    // console_new_line();

    // SchedulerProcess *next = current_cpu->current_process->next;
    // if (current_cpu->current_process != next)
    // {
    //     // There are other processes scheduled on this CPU, switch process now
    //     // TODO preserve xmm/mmx registers

    //     // Save current registers
    //     current_cpu->current_process->saved_rflags = stack->base.rflags;
    //     current_cpu->current_process->saved_instruction_pointer = stack->base.instruction_pointer;
    //     current_cpu->current_process->saved_stack_pointer = stack->base.stack_pointer;
    //     current_cpu->current_process->saved_registers = stack->registers;

    //     // Restore next process registers
    //     stack->base.instruction_pointer = next->saved_instruction_pointer;
    //     stack->base.stack_pointer = next->saved_stack_pointer;
    //     stack->base.rflags = next->saved_rflags;
    //     stack->registers = next->saved_registers;

    //     current_cpu->current_process = next;
    // }

    current_cpu->local_apic->end_of_interrupt = 0;
}

void scheduler_initialize()
{
    Cpu *cpu = cpu_get_current();

    interrupt_register(0x23, scheduler_interrupt, INTERRUPT_TYPE_INTERRUPT);
    cpu->local_apic->timer_initial_count = 100000;
    cpu->local_apic->timer_divide_config = 0b1010;
    cpu->local_apic->timer_vector = 0x23 | (1 << 17);
}

typedef void (*EntrypointFunction)();

static unsigned long current_process_id = 0;

void scheduler_execute(EntrypointFunction entrypoint)
{
    Cpu *cpu = cpu_get_current();

    SchedulerProcess *process = memory_physical_allocate();
    process->id = current_process_id++;

    // Set up page table information
    process->paging_index.level4_table = memory_physical_allocate();
    memory_zero(process->paging_index.level4_table, 4096);
    process->paging_index.level3_table = 0;
    process->paging_index.level2_table = 0;
    process->paging_index.level1_table = 0;
    process->paging_index.level4_index = 0;
    process->paging_index.level3_index = 0;
    process->paging_index.level2_index = 0;
    process->paging_index.level1_index = 0;

    if (cpu->current_process)
    {
        // Insert new process into linked list
        process->next = cpu->current_process;
        process->previous = cpu->current_process->previous;
        cpu->current_process->previous = process;
    }
    else
    {
        // This is the first process on this CPU
        process->next = process;
        process->previous = process;
        cpu->current_process = process;
    }

    process->saved_rflags = 0b1001000110; // Default flags
    memory_zero(&process->saved_registers, sizeof(SchedulerSavedRegisters));
    process->saved_stack_pointer = memory_physical_allocate();
    process->saved_instruction_pointer = entrypoint;
}