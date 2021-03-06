#include "kokos/core.h"
#include "kokos/cpu.h"
#include "kokos/memory_physical.h"
#include "kokos/paging.h"
#include "kokos/util.h"
#include "kokos/scheduler.h"
#include "kokos/console.h"
#include "kokos/idt.h"
#include "kokos/gdt.h"
#include "kokos/memory.h"

inline struct cpu_id_result cpu_id(unsigned int function)
{
    struct cpu_id_result result;
    asm volatile("cpuid"
                 : "=a"(result.eax), "=b"(result.ebx), "=c"(result.ecx), "=d"(result.edx)
                 : "a"(function)
                 :);
    return result;
}

inline unsigned long cpu_timestamp()
{
    unsigned long upper;
    unsigned long lower;
    asm volatile("rdtsc\n"
                 : "=a"(lower), "=d"(upper)
                 :
                 :);
    return lower | (upper << 32);
}

inline unsigned long cpu_wait_microsecond()
{
    unsigned long start = cpu_timestamp(), current = start;
    while (current - start < 10000ull) // ~1us
    {
        current = cpu_timestamp();
        asm volatile("pause");
    }
}

inline unsigned long cpu_wait_millisecond()
{
    unsigned long start = cpu_timestamp(), current = start;
    while (current - start < 10000000ull) // ~1ms
    {
        current = cpu_timestamp();
        asm volatile("pause");
    }
}

inline unsigned long cpu_read_msr(unsigned int register_index)
{
    unsigned int lower, upper;
    asm volatile("rdmsr"
                 : "=a"(lower), "=d"(upper)
                 : "c"(register_index)
                 :);
    return ((unsigned long)upper << 32) | (unsigned long)lower;
}

inline unsigned long cpu_write_msr(unsigned int register_index, unsigned long value)
{
    asm volatile("wrmsr" ::"a"((unsigned int)value), "d"((unsigned int)(value >> 32)), "c"(register_index)
                 :);
}

inline struct cpu *cpu_get_current()
{
    struct cpu *cpu;

    asm volatile("mov %0, [fs:0]"
                 : "=rm"(cpu)::);

    return cpu;
}

inline void cpu_panic(const char *message)
{
    asm volatile("cli");
    console_print("[cpu] PANIC! ");
    console_print(message);
    console_new_line();
    while (1)
    {
        asm volatile("hlt");
    }
}

static int current_cpu_id = 0;
extern unsigned long max_memory_address;

struct cpu *cpu_initialize(void (*entrypoint)())
{
    // The FS segment register will point to cpu-specific information
    struct cpu *cpu = memory_physical_allocate();
    cpu->address = cpu;
    cpu->id = current_cpu_id++;
    cpu->interrupt_descriptor_table = 0;
    cpu_write_msr(CPU_MSR_FS_BASE, cpu);

    console_print("[cpu] set up GDT\n");
    gdt_initialize();

    // Set up this CPU's interrupt descriptor table (IDT)
    console_print("[cpu] set up IDT\n");
    idt_initialize();

    console_print("[cpu] set up dummy process\n");

    // Create dummy process, required for paging to work
    struct scheduler_process *dummy_process = memory_physical_allocate();
    dummy_process->saved_instruction_pointer = 0;
    dummy_process->saved_stack_pointer = (unsigned char *)memory_physical_allocate() + 4096;
    dummy_process->id = 20;
    dummy_process->next = dummy_process;
    dummy_process->previous = dummy_process;

    dummy_process->paging_context.level4_table = memory_physical_allocate();
    memory_zero(dummy_process->paging_context.level4_table, 4096);
    dummy_process->paging_context.level3_table = 0;
    dummy_process->paging_context.level2_table = 0;
    dummy_process->paging_context.level1_table = 0;
    dummy_process->paging_context.level4_index = 0;
    dummy_process->paging_context.level3_index = 0;
    dummy_process->paging_context.level2_index = 0;
    dummy_process->paging_context.level1_index = 0;
    cpu->current_process = dummy_process;

    // Identity map whole RAM
    if (paging_get_hugepages_supported())
    {
        console_print("[paging] identity map using 1GiB pages\n");

        // Identity map whole memory using 1GB huge pages
        paging_map_physical_at(&dummy_process->paging_context, 0, 0, ALIGN_TO_NEXT(max_memory_address, 0x40000000ul), PAGING_FLAG_1GB | PAGING_FLAG_READ | PAGING_FLAG_WRITE);

        console_print("[paging] done\n");
    }
    else
    {
        console_print("[paging] identity map using 2MiB pages (1GiB pages not supported)\n");

        // Identity map whole memory using 2MB huge pages
        paging_map_physical_at(&dummy_process->paging_context, 0, 0, ALIGN_TO_NEXT(max_memory_address, 0x200000ul), PAGING_FLAG_2MB | PAGING_FLAG_READ | PAGING_FLAG_WRITE);

        console_print("[paging] done\n");
    }

    console_print("[cpu] setting new page table\n");

    // Now that paging should work, map the local APIC
    console_print("[cpu] mapping local APIC\n");

    unsigned long local_apic_info = cpu_read_msr(CPU_MSR_LOCAL_APIC);
    struct apic *local_apic_physical = local_apic_info & 0x000ffffffffff000;

    // Check 11th bit to check if it is enabled
    if (!(local_apic_info & 0x800))
    {
        cpu_panic("local apic not enabled, cannot enable\n");
        return;
    }

    cpu->local_apic_physical = local_apic_physical;
    if (!paging_map_physical_at(&dummy_process->paging_context, local_apic_physical, CPU_APIC_ADDRESS, sizeof(struct apic), PAGING_FLAG_WRITE | PAGING_FLAG_READ))
    {
        cpu_panic("could not map local apic");
        return;
    }

    console_print("[cpu] local_apic_physical = 0x");
    console_print_u64((unsigned long)local_apic_physical, 16);
    console_new_line();

    // Tell cpu to use new page table
    asm volatile("mov cr3, %0" ::"a"(cpu->current_process->paging_context.level4_table)
                 :);
    console_print("[cpu] apic id ");
    console_print_u64(CPU_APIC->id >> 24, 10);
    console_new_line();

    // Enable APIC after interrupt vectors were intialized
    // console_print("[cpu] enable local APIC\n");
    CPU_APIC->spurious_interrupt_vector = 0x1FF;

    // Set new stack pointer
    console_print("[cpu] allocating stack space\n");
    void *stack = (unsigned char *)paging_map(&dummy_process->paging_context, 4096ul * 8ul, PAGING_FLAG_READ | PAGING_FLAG_WRITE) + 4096ul * 8ul; // paging_map(4096 * 8, PAGING_FLAG_READ | PAGING_FLAG_WRITE);

    // Because parameters can be passed on the stack, we must assure that the entrypoint parameter is available after a stack switch
    // Store it in a register using the register keyword before the stack switch
    register void (*entrypoint_saved)() = entrypoint;
    asm volatile("mov rsp, %0" ::"rm"(stack));

    console_print("[cpu] set up scheduler\n");
    scheduler_initialize();

    // Enable hardware interrupts
    asm volatile("sti");

    if (entrypoint_saved)
    {
        entrypoint_saved();
    }

    while (1)
    {
        asm volatile("hlt");
    }
}
