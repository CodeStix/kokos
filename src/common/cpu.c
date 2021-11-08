#include "cpu.h"
#include "memory_physical.h"
#include "paging.h"
#include "../include/memory.h"

inline struct CpuIdResult cpu_id(unsigned int function)
{
    struct CpuIdResult result;
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

inline Cpu *cpu_get_current()
{
    Cpu *cpu;

    asm volatile("mov %0, [fs:0]"
                 : "=rm"(cpu)::);

    return cpu;
}

static int current_cpu_id = 0;
extern volatile unsigned long page_table_level3[512];

Cpu *cpu_initialize()
{
    unsigned long local_apic_info = cpu_read_msr(CPU_MSR_LOCAL_APIC);
    Apic *local_apic = local_apic_info & 0x000ffffffffff000;

    // Check 11th bit to check if it is enabled
    if (!(local_apic_info & 0x800))
    {
        console_print("[cpu] fatal: local apic not enabled, cannot enable\n");
        return;
    }

    console_print("[cpu] local_apic = 0x");
    console_print_u64((unsigned long)local_apic, 16);
    console_new_line();

    // The FS segment register will point to cpu-specific information
    Cpu *cpu = memory_physical_allocate();
    cpu->address = cpu;
    cpu->id = current_cpu_id++;
    cpu->interrupt_descriptor_table = 0;
    cpu_write_msr(CPU_MSR_FS_BASE, cpu);

    // Create dummy process, required for paging to work
    Process *dummy_process = memory_physical_allocate();
    dummy_process->instruction_pointer = 0;
    dummy_process->id = 0;
    dummy_process->next = dummy_process;
    dummy_process->previous = dummy_process;

    dummy_process->paging_index.level4_table = memory_physical_allocate();
    memory_zero(dummy_process->paging_index.level4_table, 4096);
    dummy_process->paging_index.level3_table = memory_physical_allocate();
    memory_zero(dummy_process->paging_index.level3_table, 4096);
    dummy_process->paging_index.level2_table = memory_physical_allocate();
    memory_zero(dummy_process->paging_index.level2_table, 4096);
    dummy_process->paging_index.level1_table = memory_physical_allocate();
    memory_zero(dummy_process->paging_index.level1_table, 4096);

    dummy_process->paging_index.level4_index = 1;
    dummy_process->paging_index.level3_index = 0;
    dummy_process->paging_index.level2_index = 0;
    dummy_process->paging_index.level1_index = 0;
    dummy_process->paging_index.level4_table[0] = (unsigned long)page_table_level3 | PAGING_ENTRY_FLAG_PRESENT | PAGING_ENTRY_FLAG_WRITABLE;
    dummy_process->paging_index.level4_table[1] = (unsigned long)dummy_process->paging_index.level3_table | PAGING_ENTRY_FLAG_PRESENT | PAGING_ENTRY_FLAG_WRITABLE;
    dummy_process->paging_index.level3_table[0] = (unsigned long)dummy_process->paging_index.level2_table | PAGING_ENTRY_FLAG_PRESENT | PAGING_ENTRY_FLAG_WRITABLE;
    dummy_process->paging_index.level2_table[0] = (unsigned long)dummy_process->paging_index.level1_table | PAGING_ENTRY_FLAG_PRESENT | PAGING_ENTRY_FLAG_WRITABLE;

    cpu->current_process = dummy_process;

    // Now that paging should work, map the local APIC
    cpu->local_apic = paging_map_physical(local_apic, sizeof(Apic), PAGING_FLAG_WRITE | PAGING_FLAG_READ);

    return cpu;
}

typedef void (*EntrypointFunction)();

static unsigned long current_process_id = 0;

void cpu_execute(EntrypointFunction entrypoint)
{
    Cpu *cpu = cpu_get_current();

    Process *process = memory_physical_allocate();
    process->id = current_process_id++;

    // Set up page table information

    memory_zero(&process->paging_index, sizeof(PagingIndex));
    process->paging_index.level4_table = memory_physical_allocate();

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

    process->stack_pointer = memory_physical_allocate();
    process->instruction_pointer = entrypoint;
}