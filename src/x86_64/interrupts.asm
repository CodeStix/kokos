extern console_print
extern console_print_u64
extern console_print_char

section .text
bits 64

print_registers:
    push rdi
    push rsi
    push rdi
    push rsi

    mov rdi, info_rax
    call console_print
    mov rdi, rax
    mov rsi, 10
    call console_print_u64

    mov rdi, info_rbx
    call console_print
    mov rdi, rbx
    mov rsi, 10
    call console_print_u64

    mov rdi, info_rcx
    call console_print
    mov rdi, rcx
    mov rsi, 10
    call console_print_u64

    mov rdi, info_rdx
    call console_print
    mov rdi, rdx
    mov rsi, 10
    call console_print_u64

    mov rdi, info_rsi
    call console_print
    pop rdi
    mov rsi, 10
    call console_print_u64

    mov rdi, info_rdi
    call console_print
    pop rdi
    mov rsi, 10
    call console_print_u64

    mov rdi, info_ebp
    call console_print
    mov rdi, rbp
    mov rsi, 10
    call console_print_u64

    mov rdi, info_esp
    call console_print
    mov rdi, rsp
    mov rsi, 10
    call console_print_u64

    mov rdi, 0xA
    call console_print_char

    pop rsi
    pop rdi
    ret

interrupt_handle_divide_zero:
    push rdi
    mov rdi, error_divide_by_zero
    call console_print  ; Notes on calling c functions from assembly (using System V X86_64 calling convention): https://wiki.osdev.org/Calling_Conventions
    call print_registers
    pop rdi
    hlt

interrupt_handle_breakpoint:
    push rdi
    mov rdi, error_breakpoint
    call console_print  
    call print_registers
    pop rdi
    iretq

interrupt_handle_overflow:
    push rdi
    mov rdi, error_overflow
    call console_print  
    pop rdi
    iretq

interrupt_handle_bound_range:
    push rdi
    mov rdi, error_bound_range
    call console_print  
    pop rdi
    hlt

interrupt_handle_invalid_opcode:
    push rdi
    mov rdi, error_invalid_opcode
    call console_print  
    pop rdi
    hlt

interrupt_handle_double_fault:
    push rdi
    mov rdi, error_double_fault
    call console_print  
    pop rdi
    hlt

interrupt_handle_invalid_tss:
    push rdi
    mov rdi, error_invalid_tss
    call console_print  
    pop rdi
    hlt

interrupt_handle_segment_not_found:
    push rdi
    mov rdi, error_invalid_tss
    call console_print  
    pop rdi
    hlt

interrupt_handle_stack:
    push rdi
    mov rdi, error_stack
    call console_print  
    pop rdi
    hlt

interrupt_handle_general_protection:
    push rdi
    mov rdi, error_general_protection
    call console_print  
    pop rdi
    hlt

interrupt_handle_page_fault:
    push rdi

    mov rdi, error_page_fault
    call console_print  

    mov rdi, cr2                ; Address that caused the page fault is stored in cr2
    mov rsi, 16
    call console_print_u64

    mov rdi, 0x16
    call console_print_char

    pop rdi
    hlt

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
    register_interrupt_handler 4, interrupt_handle_overflow
    register_interrupt_handler 5, interrupt_handle_bound_range
    register_interrupt_handler 6, interrupt_handle_invalid_opcode
    register_interrupt_handler 8, interrupt_handle_double_fault
    register_interrupt_handler 10, interrupt_handle_invalid_tss
    register_interrupt_handler 11, interrupt_handle_segment_not_found
    register_interrupt_handler 12, interrupt_handle_stack
    register_interrupt_handler 13, interrupt_handle_general_protection
    register_interrupt_handler 14, interrupt_handle_page_fault
    lidt [idt64.pointer]
    ret


section .data ; The data section contains initialized data

error_divide_by_zero:
    db "error: divide by zero!", 0xA, 0
error_breakpoint:
    db "warning: hit breakpoint!", 0xA, 0
error_overflow:
    db "error: overflow!", 0xA, 0
error_bound_range:
    db "error: bound range!", 0xA, 0
error_invalid_opcode:
    db "error: invalid opcode!", 0xA, 0
error_double_fault:
    db "error: double fault!", 0xA, 0
error_invalid_tss:
    db "error: invalid task state segment!", 0xA, 0
error_segment_not_present:
    db "error: segment not present!", 0xA, 0
error_stack:
    db "error: stack problem!", 0xA, 0
error_general_protection:
    db "error: generatal protection exception!", 0xA, 0
error_page_fault:
    db "error: page fault! process tried to access address 0x", 0
info_registers:
    db "registers", 0
info_rax:
    db "rax=", 0
info_rbx:
    db ", rbx=", 0
info_rcx:
    db ", rcx=", 0
info_rdx:
    db ", rdx=", 0
info_rdi:
    db ", rdi=", 0
info_rsi:
    db ", rsi=", 0
info_esp:
    db ", rsp=", 0
info_ebp:
    db ", rbp=", 0

idt64:
    resb 16 * 256 - ($ - idt64) ; Reserve the whole idt, each entry is 16 bytes
.pointer:
    dw $ - idt64 - 1
    dq idt64
