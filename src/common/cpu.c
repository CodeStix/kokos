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

inline unsigned long cpu_wait()
{
    unsigned long start = cpu_timestamp(), current = start;
    while (current - start < 10000ull) // ~1us
    {
        current = cpu_timestamp();
        asm volatile("pause");
    }
}