; This file is currently unused, but is kept for the future
; Because multiboot2 is used, the kernel is loaded in protected mode (32 bit), not real mode (16 bit)

section .text

; Tell nasm that 16 bit assembly must be generated below
bits 16

start16:
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov esp, 512

    mov di, 0
    mov si, memory_map
    xor ebx, ebx
    mov edx, 0x534D4150
    mov ecx, 24
    mov eax, 0xE820
    int 0x15
    ; mov ah, 0x0e
    ; mov al, '!'
    ; int 0x10

    mov eax, cr0         ; Go back to 32 bit mode
    or eax, 0b1
    mov cr0, eax
    ret