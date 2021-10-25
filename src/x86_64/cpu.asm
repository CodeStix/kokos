global cpu_startup
global cpu_startup_increment
global cpu_startup_mode

section .lowtext

align 4096
bits 16

cpu_startup:
    cli

    inc word [cpu_startup_increment]  ; Notify boot processor that this processor has been enabled, it waits for this variable to be non-zero

    mov eax, cr0 ; Enter 32 bit protected mode
    or al, 1
    mov cr0, eax


.loop:
    hlt
    jmp .loop

cpu_startup_mode:
    dd 0

cpu_startup_increment:
    dw 0