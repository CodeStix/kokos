#pragma once
#include "kokos/idt.h"
#include "kokos/paging.h"
#include "kokos/apic.h"

#define SCHEDULER_TIMER_INTERVAL 10000

// See schedule.asm
struct scheduler_saved_registers
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
};

struct scheduler_interrupt_frame
{
    struct scheduler_saved_registers registers;
    IdtFrame base;
};

struct scheduler_process
{
    // The id of this process
    unsigned long id;
    // Pointer to the next process
    struct scheduler_process *next;
    // Pointer to the previous process
    struct scheduler_process *previous;
    // Pointer to the pages table used by this process
    PagingContext paging_context;
    // Virtual address to the local apic
    Apic *local_apic;

    // TODO move to thread struct
    unsigned long saved_rflags;
    // Address of the current instruction where it left off
    void *saved_instruction_pointer;
    // Address of the stack
    void *saved_stack_pointer;
    // Saved registers during a task switch
    struct scheduler_saved_registers saved_registers;
};

void scheduler_initialize();

void scheduler_execute(void (*scheduler_entrypoint)());