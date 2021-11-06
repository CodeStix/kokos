global interrupt_schedule
extern interrupt_schedule_handler

bits 64
section .text

interrupt_schedule:
    push rax
    push rbx ; Test: will be preserved?
    push rcx
    push rdx
    push rdi
    push rsi
    push r8
    push r9
    push r10
    push r11    

    mov rdi, rsp
    sub rdi, 8 * 10
    cld 
    call interrupt_schedule_handler

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
    iretq