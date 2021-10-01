extern kernel_main
global start64

section .text
bits 64     ; Tell the assembler that this file should create 64 bit instructions

start64:
    mov ax, 0
    mov ss, ax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ; mov dword [0xb8000], 0x2f4b2f4f
    call kernel_main

    hlt