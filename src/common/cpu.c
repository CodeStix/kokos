#include "cpu.h"

inline CpuIdResult cpu_id(unsigned int function)
{
    CpuIdResult result;
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
    Cpu *cpu_info = memory_physical_allocate();
    cpu_info->address = cpu_info;
    cpu_info->id = current_cpu_id++;
    cpu_info->local_apic;
    cpu_info->interrupt_descriptor_table = 0;
    cpu_write_msr(CPU_MSR_FS_BASE, cpu_info);

    return cpu_info;
}