extern kernel_main
extern gdt64.code_segment
extern console_print
global start64
global hit_breakpoint

hit_breakpoint:
    int3
    ret

%include "src/x86_64/interrupts.asm"

section .text
bits 64     ; Tell the assembler that this file should create 64 bit instructions

start64:
    call load_idt

    mov ax, 0       ; Clear registers before entering c
    mov ss, ax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    call kernel_main
    hlt


