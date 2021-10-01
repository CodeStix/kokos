; Give other files access to the 'start' address
global start

; Mark the following code to be in the text section, which typically stores code.
section .text

; Tell nasm that 32 bit assembly must be generated below
bits 32

; Name the following block of code 'start', when refernced, start will contain the address of its first instruction (mov)
start:
    mov esp, stack_top

    call check_cpuid_supported

    mov dword [0xb8000], 0x2f4b2f4f
    hlt

; Check if cpuid is supported (ap21)
; You can check if cpuid is supported by trying to flip the identification flag in the flags register, if this flag stays flipped,
; cpuid is supported.
check_cpuid_supported:
    pushfd              ; Push the flags register and pop into eax
    pop eax
    mov ecx, eax        ; Make a copy of the flags register in ecx
    xor eax, 1 << 21    ; Try to flip the id flag (ap18) and store back in flags register
    push eax
    popfd
    pushfd              ; Push the flags register and pop into eax
    pop eax
    push ecx            ; Revert flags register flip
    popfd
    cmp eax, ecx        ; Check if flip was successful, it should be flipped
    je .no_cpuid
    ret
.no_cpuid:
    mov dword [0xb8000], 0x2f302f45
    hlt

; A bbs section contains uninitialized variables, so the stack too
section .bss

stack_bottom:
    ; Reserve 1024 * 16 bytes
    resb 1024 * 16
stack_top: