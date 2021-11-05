#include "../include/cpu.h"

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