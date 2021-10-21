extern console_print
extern console_print_u64
extern console_print_char
extern console_new_line
extern interrupt_handle
global interrupt_descriptor_table

section .text
bits 64

; print_registers:
;     push rbp
;     push rsp
;     push r15
;     push r14
;     push r13
;     push r12
;     push r11
;     push r10
;     push r9
;     push r8
;     push rsi
;     push rdi
;     push rdx
;     push rcx
;     push rbx
;     push rax    

;     mov rcx, 0
; .loop:
;     mov rax, rcx
;     imul rax, 5
;     add rax, info_all_registers
;     mov rdi, rax
;     push rcx
;     call console_print
;     pop rcx

;     pop rdi
;     mov rsi, 16
;     push rcx
;     call console_print_u64
;     call console_new_line
;     pop rcx

;     inc rcx
;     cmp rcx, 16
;     jne .loop
;     ret

load_idt:
    call interrupt_register_vectors
    lidt [interrupt_descriptor_table.pointer]
    ret

interrupt_register_vectors:
    mov rcx, 0
.loop:
    mov rax, rcx
    imul rax, 16        ; shl rax, 4
    add rax, interrupt_descriptor_table
    mov rbx, rcx                                ; https://wiki.osdev.org/Interrupt_Descriptor_Table
    imul rbx, interrupt_handler.size
    add rbx, interrupt_handler
    mov word [rax], bx                          ; Interrupt handler address (first 16 bits)
    mov word [rax + 2], gdt64.code_segment      ; GDT selector
    mov byte [rax + 4], 0                       ; Interrupt Stack Table (ist) offset (not used) 
    mov byte [rax + 5], 0b1_00_0_1111           ; Type and attributes, present=1, privilegelevel=00, unused=0, type=1110 (interrupt gate) or 1111 (trap gate)
    shr rbx, 16                                 ; Interrupt gates are initiated by hardware and trap gates by software
    mov word [rax + 6], bx                      ; Interrupt handler address (second 16 bits)
    shr rbx, 16
    mov dword [rax + 8], ebx                    ; Interrupt handler address (last 32 bits)
    inc rcx
    cmp rcx, 256
jne .loop
    ret

interrupt_handler:
    %assign i 0
    %rep 256        
    push rax        ; This code is repeated 256 times (for each interrupt vector)
    push rbx        ; The rbp, rsp, r12, r13, r14 and r15 registers are automatically preserved (they are saved/not used in C code)
    push rcx        ; https://wiki.osdev.org/Interrupt_Service_Routines
    push rdx
    push rdi
    push rsi
    push r8
    push r9
    push r10
    push r11
    mov rdi, i                  
    cld
    call interrupt_handle
    pop r11
    pop r10
    pop r9
    pop r8
    pop rsi
    pop rdi
    pop rdx
    pop rcx
    pop rbx
    pop rax
    iretq
    %assign i i+1
    %endrep
.end:
.size: equ (.end - interrupt_handler) / 256

section .data ; The data section contains initialized data

interrupt_descriptor_table:
    resb 16 * 256 - ($ - interrupt_descriptor_table) ; Reserve the whole idt, each entry is 16 bytes
.pointer:
    dw $ - interrupt_descriptor_table - 1
    dq interrupt_descriptor_table

section .rodata

; info_registers:
;     db "registers", 0
; info_all_registers:
;     db "rax=", 0    ; Each register string is 5 bytes
;     db "rbx=", 0
;     db "rcx=", 0
;     db "rdx=", 0
;     db "rdi=", 0
;     db "rsi=", 0
;     db " r8=", 0
;     db " r9=", 0
;     db "r10=", 0
;     db "r11=", 0
;     db "r12=", 0
;     db "r13=", 0
;     db "r14=", 0
;     db "r15=", 0
;     db "rsp=", 0
;     db "rbp=", 0
