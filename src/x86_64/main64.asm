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
    mov ax, 0       ; Clear segment registers
    mov ss, ax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

;     pushfq
;     pop rax
;     test rax, 0x0200
;     jz .no_interrupt
;     mov rdi, info_interrupt_enabled
;     call console_print
;     jmp .end
; .no_interrupt:
;     mov rdi, info_interrupt_disabled
;     call console_print
; .end:
;     hlt

    call kernel_main
    hlt

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

