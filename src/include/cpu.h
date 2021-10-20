#pragma once

#define CPU_ID_FUNCTION_0 0
#define CPU_ID_1GB_PAGES_EDX 1 << 26
#define CPU_ID_LONG_MODE_EDX 1 << 29

typedef struct
{
    unsigned int eax, ebx, ecx, edx;
} CpuIdResult;

CpuIdResult cpu_id(unsigned int function);