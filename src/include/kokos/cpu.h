#pragma once
#include "kokos/apic.h"
#include "kokos/interrupt.h"
#include "kokos/paging.h"
#include "kokos/scheduler.h"

#define CPU_ID_FUNCTION_0 0
#define CPU_ID_1GB_PAGES_EDX 1 << 26
#define CPU_ID_LONG_MODE_EDX 1 << 29

#define CPU_MSR_LOCAL_APIC 0x0000001B
#define CPU_MSR_FS_BASE 0xC0000100
#define CPU_MSR_GS_BASE 0xC0000101

typedef struct CpuIdResult
{
    unsigned int eax, ebx, ecx, edx;
} CpuIdResult;

typedef struct Cpu
{
    // Pointer to itself (trick for cpu_get_current and cpu_initialize)
    struct Cpu *address;
    // The processor's id
    unsigned int id;
    // Pointer to this processor's local APIC
    Apic *local_apic;
    // Pointer to its interrupt descriptor table
    InterruptDescriptor *interrupt_descriptor_table;
    // Pointer to currently running process
    SchedulerProcess *current_process;
} Cpu;

// https://wiki.osdev.org/GDT
typedef struct GlobalDescriptor
{
    union
    {
        struct
        {
            unsigned short limit1 : 16;
            unsigned short base1 : 16;
            unsigned char base2 : 8;

            unsigned char accessed : 1;
            unsigned char read_write : 1;
            unsigned char direction_conforming : 1;
            unsigned char executable : 1;
            unsigned char type : 1;
            unsigned char privilege : 2;
            unsigned char present : 1;

            unsigned char limit2 : 4;
            unsigned char unused1 : 1;
            unsigned char long_mode : 1;
            unsigned char size : 1;
            unsigned char granularity : 1;

            unsigned char base3 : 8;
        };
        unsigned long as_long;
    };
} ATTRIBUTE_PACKED GlobalDescriptor;

typedef struct CpuTaskState
{
    unsigned int unused0;
    unsigned long rsp[3];
    unsigned int unused1;
    unsigned int unused2;
    unsigned long ist[7];
    unsigned int unused3;
    unsigned int unused4;
    unsigned int iopb_offset;
} ATTRIBUTE_PACKED CpuTaskState;

// Performs an cpuid instruction and returns the result
CpuIdResult cpu_id(unsigned int function);

// Returns the cpu's time stamp counter
unsigned long cpu_timestamp();

// Waits for a minimum amount of 1 microseconds
unsigned long cpu_wait_microsecond();

// Waits for a minimum amount of 1 milliseconds
unsigned long cpu_wait_millisecond();

// Reads from a model specific register
unsigned long cpu_read_msr(unsigned int register_index);

// Writes to a model specific register
unsigned long cpu_write_msr(unsigned int register_index, unsigned long value);

// Returns os-relevant information about the current cpu.
// cpu_initialize must be called first!
Cpu *cpu_get_current();

// Initializes the current cpu info.
// memory_physical_initialize and paging_initialize must be called first!
Cpu *cpu_initialize(SchedulerEntrypoint entrypoint);