#pragma once
#include "kokos/idt.h"
#include "kokos/paging.h"

#define SCHEDULER_TIMER_INTERVAL 10000

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
    unsigned long rbp;
} SchedulerSavedRegisters;

typedef struct
{
    SchedulerSavedRegisters registers;
    IdtFrame base;
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
    PagingContext paging_context;

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

void scheduler_initialize();

void scheduler_execute(SchedulerEntrypoint entrypoint);