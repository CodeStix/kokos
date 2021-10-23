extern kernel_main
extern gdt64.code_segment
extern console_print
extern console_clear
extern console_print_u64
extern console_new_line
extern console_print_length
global memory_chunk
global start64
global hit_breakpoint
global hugepages_supported

%include "src/x86_64/pci.asm"

section .text
bits 64     ; Tell the assembler that this file should create 64 bit instructions

start64:
    mov ax, 0           ; Clear segment registers
    mov ss, ax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax      
    call kernel_main    ; Enter C code (main.c)
.loop:
    hlt                 ; Halt does not shut down the processor, it can still receive interrupts. When an interrupt was handled, it continues after halt
    jmp .loop


section .bss

memory_chunk:

section .rodata

info_done:
    db "entering c", 0xA, 0
info_interrupt_enabled:
    db "interrupts enabled", 0xA, 0
info_interrupt_disabled:
    db "interrupts disabled", 0xA, 0
info_load_idt:
    db "setting up interrupts", 0xA, 0

