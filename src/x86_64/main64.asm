extern kernel_main
extern gdt64.code_segment
extern console_print
extern console_clear
extern console_print_u64
extern console_new_line
extern console_print_length
global start64
global hit_breakpoint

%include "src/x86_64/interrupts.asm"
%include "src/x86_64/pci.asm"

section .text
bits 64     ; Tell the assembler that this file should create 64 bit instructions

hit_breakpoint:
    int3
    ret

start64:
    call console_clear

    mov rdi, info_load_idt
    call console_print

    call load_idt

    mov rdi, info_done
    call console_print

    mov ax, 0       ; Clear registers before entering c
    mov ss, ax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    call kernel_main
    hlt


section .rodata

info_done:
    db "entering c", 0xA, 0
info_load_idt:
    db "setting up interrupts", 0xA, 0

