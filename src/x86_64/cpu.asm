global cpu_startup16
global cpu_startup_increment
extern page_table_level4
extern console_print
extern cpu_entrypoint

section .lowtext

align 4096
bits 16

cpu_startup16:
    cli
    mov esp, stack_top

    mov eax, cr0 ; Enter 32 bit protected mode
    or al, 1
    mov cr0, eax

    lgdt [gdt32.pointer]

    mov ax, 0x10
    mov ss, ax
    mov gs, ax
    mov ds, ax
    mov fs, ax
    mov es, ax
    call 0x8:cpu_startup32

bits 32

cpu_startup32:
    mov eax, cr0                ; Disable paging
    and eax, ~(1 << 31)
    mov cr0, eax

    mov eax, cr4                ; Enable PAE (extended pages, required for long mode, each page table entry will be 64 bit now instead of 32)
    or eax, 1 << 5
    mov cr4, eax

    mov eax, page_table_level4  ; Store the address of the 4th level table into cr3
    mov cr3, eax

    mov ecx, 0xC0000080         ; Enable long mode https://wiki.osdev.org/CPU_Registers_x86-64#IA32_EFER
    rdmsr
    or eax, 1 << 8              ; Enable long mode by toggling the 8th bit in the EFER register
    wrmsr

    mov eax, cr0                ; Enable paging
    or eax, 1 << 31
    mov cr0, eax

    lgdt [gdt64.pointer]

    mov ax, 0x10
    mov ss, ax
    mov gs, ax
    mov ds, ax
    mov fs, ax
    mov es, ax
    call 0x8:cpu_startup64


bits 64

cpu_startup64:
    call cpu_entrypoint
    mov word [cpu_startup_increment], 1  ; Notify boot processor that this processor has been enabled, it waits for this variable to be non-zero
.loop:
    hlt
    jmp .loop

cpu_startup_increment:
    dw 0

gdt32:
    dq 0          ; Null entry

    dw 0xFFFF     ; Limit all ones
    dw 0
    db 0
    db 0b10011010 ; Executable, read/write, present, code segment
    db 0b11001111 ; Limit all ones, use 4096 bytes limit unit, 32 bit  
    db 0 

    dw 0xFFFF     ; Limit all ones
    dw 0
    db 0
    db 0b10010010 ; Read/write, present, data segment
    db 0b10001111 ; Limit all ones, use 4096 bytes limit unit
    db 0 
.pointer:
    dw $ - gdt32 - 1
    dd gdt32

gdt64:
    dq 0          ; Null entry

    dw 0xFFFF     ; Limit all ones
    dw 0
    db 0
    db 0b10011010 ; Executable, read/write, present, code segment
    db 0b10101111 ; Limit all ones, use 4096 bytes limit unit, 64 bit  
    db 0 

    dw 0xFFFF     ; Limit all ones
    dw 0
    db 0
    db 0b10010010 ; Read/write, present, data segment
    db 0b10001111 ; Limit all ones, use 4096 bytes limit unit
    db 0 
.pointer:
    dw $ - gdt64 - 1
    dd gdt64


align 16

stack_bottom:
    resb 1024
stack_top:

info_done:
    db "[smp] message from other core", 0xA, 0