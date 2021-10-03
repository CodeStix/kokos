extern console_print

section .text
bits 64

interrupt_handle_divide_zero:
    mov rdi, error_divide_by_zero
    call console_print  ; Notes on calling c functions from assembly (using System V X86_64 calling convention): https://wiki.osdev.org/Calling_Conventions
    hlt
    ; iretq

interrupt_handle_breakpoint:
    mov rdi, error_breakpoint
    call console_print  
    iretq

%macro register_interrupt_handler 2             
    mov rax, %1
    imul rax, 16        ; shl rax, 4
    add rax, idt64
    mov rbx, %2                                 ; https://wiki.osdev.org/Interrupt_Descriptor_Table
    mov word [rax], bx                          ; Interrupt handler address (first 16 bits)
    mov word [rax + 2], gdt64.code_segment      ; GDT selector
    mov byte [rax + 4], 0                       ; Interrupt Stack Table (ist) offset (not used) 
    mov byte [rax + 5], 0b1_00_0_1111           ; Type and attributes, present=1, privilegelevel=00, unused=0, type=1110 (interrupt gate) or 1111 (trap gate)
    shr rbx, 16                                 ; Interrupt gates are initiated by hardware and trap gates by software
    mov word [rax + 6], bx                      ; Interrupt handler address (second 16 bits)
    shr rbx, 16
    mov dword [rax + 8], ebx                    ; Interrupt handler address (last 32 bits)
%endmacro

load_idt:
    register_interrupt_handler 0, interrupt_handle_divide_zero
    register_interrupt_handler 3, interrupt_handle_breakpoint
    lidt [idt64.pointer]
    ret


section .data ; The data section contains initialized data

error_divide_by_zero:
    db "error: divide by zero!", 0xA, 0
error_breakpoint:
    db "warning: hit breakpoint!", 0xA, 0

idt64:
    resb 16 * 256 - ($ - idt64) ; Reserve the whole idt, each entry is 16 bytes

.pointer:
    dw $ - idt64 - 1
    dq idt64
