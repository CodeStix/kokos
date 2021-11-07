#pragma once
#include "apic.h"
#include "interrupt.h"

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

typedef struct Process
{
    // The id of this process
    unsigned long id;
    // Pointer to the next process
    struct Process *next;
    // Pointer to the previous process
    struct Process *previous;
    // Pointer to the page table used by this process
    unsigned long *topmost_page_table;

    // TODO move to thread struct
    // Address of the current instruction where it left off
    void *instruction_pointer;
    // Address of the stack
    void *stack_pointer;
    // Saved registers during a task switch
    unsigned long saved_registers[10];
} Process;

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
    Process *current_process;
} Cpu;

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
Cpu *cpu_initialize();