#pragma once
#include "interrupt.h"
#include "paging.h"

typedef struct
{
    unsigned long registers[10];
    InterruptFrame base;
} SchedulerInterruptFrame;

typedef struct Process
{
    // The id of this process
    unsigned long id;
    // Pointer to the next process
    struct Process *next;
    // Pointer to the previous process
    struct Process *previous;
    // Pointer to the pages table used by this process
    PagingIndex paging_index;

    // TODO move to thread struct
    // Address of the current instruction where it left off
    void *instruction_pointer;
    // Address of the stack
    void *stack_pointer;
    // Saved registers during a task switch
    unsigned long saved_registers[10];
} Process;