#pragma once

typedef struct
{
    unsigned int eax, ebx, ecx, edx;
} CpuIdResult;

CpuIdResult cpu_id(unsigned int function);