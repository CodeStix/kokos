global apic_disable_pic
global apic_check_supported

section .text
bits 64

; TODO check apic support using acpi MADT table

apic_disable_pic:
    mov al, 0xff
    out 0xa1, al
    out 0x21, al
    ret

apic_check_supported:
    push rbx    
    mov rax, 0x1            ; Execute function 1 of cpuid, it must be stored in rax
    cpuid
    pop rbx
    test rdx, 0b1_00000000  ; Test for bit 9 in the rdx register, which means that an apic is supported (amd64 vol3 p574)
    jz .not_supported
    mov rax, 1
    ret
.not_supported:
    mov rax, 0
    ret

    
