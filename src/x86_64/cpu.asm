global cpu_startup
global cpu_startup_increment

section .lowtext

align 4096
bits 16

cpu_startup:
    ; mov dword [0xb8000], 0x4f304f45 ; Print E0 in red to screen https://wiki.osdev.org/Printing_To_Screen
    inc word [cpu_startup_increment]
.loop:
    hlt
    jmp .loop

cpu_startup_increment:
    dw 0