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
    call check_long_mode

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
    mov dword [0xb8000], 0x4f304f45 ; Print E0 in red to screen https://wiki.osdev.org/Printing_To_Screen
    hlt

check_long_mode:
    mov eax, 0x80000000     ; Call cpuid with parameter 0x80000000 https://en.wikipedia.org/wiki/CPUID
    cpuid
    cmp eax, 0x80000001     ; The returned value should at least be 0x80000001 because we need that
    jb .no_long_mode        ; function to check if long mode is supported.

    mov eax, 0x80000001     ; Call cpuid with parameter 0x80000001 https://en.wikipedia.org/wiki/CPUID
    cpuid
    test edx, 1 << 29       ; Check for long mode flag (al155)
    jz .no_long_mode
    ret

.no_long_mode:
    mov dword [0xb8000], 0x4f314f45 ; Print E1 in red to screen https://wiki.osdev.org/Printing_To_Screen
    hlt

; A bbs section contains uninitialized variables, so the stack too
section .bss

stack_bottom:
    ; Reserve 1024 * 16 bytes
    resb 1024 * 16
stack_top: