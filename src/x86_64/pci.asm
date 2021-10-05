extern console_print
extern console_new_line

section .text
bits 64

read_device:                ; https://wiki.osdev.org/PCI
    mov rdi, 0              ; Contains bus number
    mov rsi, 0              ; Contains slot number
    mov rdx, 0              ; Contains function number
    mov rcx, 0              ; Contains which part of the pci register to read (must be a multiple of 4)

    mov rax, 1 << 31        ; The enable bit at position 31

    and rdi, 0b11111111     ; The bus number at position 16, is 8 bits in size
    shl rdi, 16             
    or rax, rdi              
    
    and rsi, 0b11111        ; The slot number at position 11, is 5 bits in size
    shl rsi, 11             
    or rax, rsi

    and rdx, 0b111          ; The function number at position 8, is 3 bits in size
    shl rdx, 8
    or rax, rdx

    and rcx, 0b11111100     ; Which part of the pci regsister

    out 0xCF8, eax,         ; Send which part of pci configuration space to read

    mov eax, 0
    in eax, 0xCFC           ; Read pci configuration space

    ret

section .rodata

error_invalid_rdsp:
    db "halt: rdsp has wrong checksum!", 0xA, 0x0