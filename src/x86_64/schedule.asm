global scheduler_interrupt
extern scheduler_handle_interrupt

bits 64
section .text

scheduler_interrupt:
    push rbp    ; rsp + 80
    push rax    ; rsp + 72
    push rbx    ; rsp + 64 Test: will be preserved?
    push rcx    ; rsp + 56
    push rdx    ; rsp + 48
    push rdi    ; rsp + 40
    push rsi    ; rsp + 32
    push r8     ; rsp + 24
    push r9     ; rsp + 16
    push r10    ; rsp + 8
    push r11    ; rsp    

    mov rdi, rsp
    ; Same as
    ; mov rdi, rsp
    ; add rdi, 8 * 10
    cld 
    call scheduler_handle_interrupt

    pop r11
    pop r10
    pop r9
    pop r8
    pop rsi
    pop rdi
    pop rdx
    pop rcx
    pop rbx ; Test: will be preserved?
    pop rax
    pop rbp
    iretq