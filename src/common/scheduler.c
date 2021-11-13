#include "kokos/scheduler.h"
#include "kokos/cpu.h"
#include "kokos/console.h"
#include "kokos/memory.h"
#include "kokos/memory_physical.h"
#include "kokos/idt.h"
#include "kokos/lock.h"
#include "kokos/util.h"

// Defined in src/x86_64/schedule.asm, this assembly code calls scheduler_handle_interrupt below
extern void(scheduler_interrupt)();

unsigned int print_lock = 0;
unsigned int counters[16] = {0};

void scheduler_handle_interrupt(SchedulerInterruptFrame *stack)
{
    Cpu *current_cpu = cpu_get_current();
    // Allow more interrupts to be handled
    current_cpu->local_apic->end_of_interrupt = 0;

    SchedulerProcess *next = current_cpu->current_process->next;
    if (current_cpu->current_process != next)
    {
        // There are other processes scheduled on this CPU, switch process now
        // TODO preserve xmm/mmx registers

        // console_print("next #");
        // console_print_u64(next->id, 10);
        // console_print(" rip 0x");
        // console_print_u64(current_cpu->current_process->saved_instruction_pointer, 16);
        // console_print(" -> 0x");
        // console_print_u64(next->saved_instruction_pointer, 16);
        // console_print(" rsp 0x");
        // console_print_u64(current_cpu->current_process->saved_stack_pointer, 16);
        // console_print(" -> 0x");
        // console_print_u64(next->saved_stack_pointer, 16);
        // console_new_line();

        // Save current registers
        current_cpu->current_process->saved_rflags = stack->base.rflags;
        current_cpu->current_process->saved_instruction_pointer = stack->base.instruction_pointer;
        current_cpu->current_process->saved_stack_pointer = stack->base.stack_pointer;
        current_cpu->current_process->saved_registers = stack->registers;

        // Restore next process registers
        stack->base.instruction_pointer = next->saved_instruction_pointer;
        stack->base.stack_pointer = next->saved_stack_pointer;
        stack->base.rflags = next->saved_rflags;
        stack->registers = next->saved_registers;

        current_cpu->current_process = next;
    }

    // Restart timer
    current_cpu->local_apic->timer_initial_count = SCHEDULER_TIMER_INTERVAL;
}

void scheduler_initialize()
{
    Cpu *cpu = cpu_get_current();

    // Register the local APIC timer, see https://kokos.run/#WzAsIkFNRDY0Vm9sdW1lMi5wZGYiLDY1NyxbNjU3LDMxLDY1NywzMV1d
    idt_register_interrupt(0x23, scheduler_interrupt, IDT_GATE_TYPE_TRAP, IDT_STACK_SCHEDULER);
    cpu->local_apic->timer_initial_count = SCHEDULER_TIMER_INTERVAL;
    // Use divisor 128
    cpu->local_apic->timer_divide_config = 0b1010;
    // Use one-shot mode, when an scheduler interrupt is done, it re-enables the one-shot mode.
    cpu->local_apic->timer_vector = 0x23;
}

static unsigned long current_process_id = 0;

extern unsigned long max_memory_address;

void scheduler_execute(SchedulerEntrypoint entrypoint)
{
    Cpu *cpu = cpu_get_current();

    SchedulerProcess *process = memory_physical_allocate();
    process->id = current_process_id++;

    // Set up page table information
    process->paging_context.level4_table = memory_physical_allocate();
    memory_zero(process->paging_context.level4_table, 4096);
    process->paging_context.level3_table = 0;
    process->paging_context.level2_table = 0;
    process->paging_context.level1_table = 0;
    process->paging_context.level4_index = 0;
    process->paging_context.level3_index = 0;
    process->paging_context.level2_index = 0;
    process->paging_context.level1_index = 0;

    // Identity map RAM
    if (paging_get_hugepages_supported())
    {
        console_print("[paging] identity map using 1GiB pages\n");

        // Identity map whole memory using 1GB huge pages
        paging_map_physical_at(&process->paging_context, 0, 0, ALIGN_TO_NEXT(max_memory_address, 0x40000000ul), PAGING_FLAG_1GB | PAGING_FLAG_READ | PAGING_FLAG_WRITE);

        console_print("[paging] done\n");
    }
    else
    {
        console_print("[paging] identity map using 2MiB pages (1GiB pages not supported)\n");

        // Identity map whole memory using 2MB huge pages
        paging_map_physical_at(&process->paging_context, 0, 0, ALIGN_TO_NEXT(max_memory_address, 0x200000ul), PAGING_FLAG_2MB | PAGING_FLAG_READ | PAGING_FLAG_WRITE);

        console_print("[paging] done\n");
    }

    process->saved_rflags = 0b1001000110; // Default flags
    memory_zero(&process->saved_registers, sizeof(SchedulerSavedRegisters));
    process->saved_stack_pointer = (unsigned char *)memory_physical_allocate() + 4096; //(unsigned char *)paging_map(4096 * 8, PAGING_FLAG_READ | PAGING_FLAG_WRITE) + 4096 * 8;
    process->saved_instruction_pointer = entrypoint;

    // Temporary disable scheduler interrupt
    asm volatile("cli");

    // Insert new process into linked list
    if (cpu->current_process)
    {
        // Insert new process into linked list
        process->next = cpu->current_process->next;
        process->previous = cpu->current_process;
        cpu->current_process->next = process;
    }
    else
    {
        // This is the first process on this CPU
        process->next = process;
        process->previous = process;
        cpu->current_process = process;
    }

    asm volatile("sti");
}