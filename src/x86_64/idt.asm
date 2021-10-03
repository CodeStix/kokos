extern console_print

section .text
bits 64

handle_divide_zero:
    mov rdi, error_divide_by_zero
    call console_print  ; Notes on calling c functions from assembly (using System V X86_64 calling convention): https://wiki.osdev.org/Calling_Conventions
    hlt
    ; iretq

load_idt:
    mov rax, handle_divide_zero
    mov word [idt64.irq0], ax
    shr rax, 16
    mov word [idt64.irq0_offset2], ax
    shr rax, 16
    mov dword [idt64.irq0_offset3], eax
    lidt [idt64.pointer]
    ret


section .data ; The data section contains initialized data

error_divide_by_zero:
    db "error: divide by zero!", 0xA, 0

idt64:
.irq0:                      ; https://wiki.osdev.org/Interrupt_Descriptor_Table
    dw 0                    ; Interrupt handler address
    dw gdt64.code_segment   ; GDT selector
    db 0                    ; Interrupt Stack Table (ist) offset (not used) 
    db 0b1_00_0_1111        ; present=1, privilegelevel=00, unused=0, type=1110 (interrupt gate) or 1111 (trap gate)
                            ; Interrupt gates are initiated by hardware and trap gates by software
.irq0_offset2:
    dw 0
.irq0_offset3:
    dd 0
    dd 0

    resb 16 * 256 - ($ - idt64) ; Reserve the whole idt, each entry is 16 bytes

.pointer:
    dw $ - idt64 - 1
    dq idt64
