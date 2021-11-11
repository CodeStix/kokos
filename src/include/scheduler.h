#pragma once
#include "interrupt.h"
#include "paging.h"

// See schedule.asm
typedef struct SchedulerSavedRegisters
{
    unsigned long r11;
    unsigned long r10;
    unsigned long r9;
    unsigned long r8;
    unsigned long rsi;
    unsigned long rdi;
    unsigned long rdx;
    unsigned long rcx;
    unsigned long rbx;
    unsigned long rax;
} SchedulerSavedRegisters;

typedef struct
{
    SchedulerSavedRegisters registers;
    InterruptFrame base;
} SchedulerInterruptFrame;

typedef struct SchedulerProcess
{
    // The id of this process
    unsigned long id;
    // Pointer to the next process
    struct SchedulerProcess *next;
    // Pointer to the previous process
    struct SchedulerProcess *previous;
    // Pointer to the pages table used by this process
    PagingIndex paging_index;

    // TODO move to thread struct
    unsigned long saved_rflags;
    // Address of the current instruction where it left off
    void *saved_instruction_pointer;
    // Address of the stack
    void *saved_stack_pointer;
    // Saved registers during a task switch
    SchedulerSavedRegisters saved_registers;
} SchedulerProcess;

typedef void (*SchedulerEntrypoint)();