
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

search_rsdp:                        ; The Root System Description Pointer is a structure found in memory, between address 0x000E0000 and 0x000FFFFF or between 0x00080000 and 0x0009FFFF
    mov rcx, 0x00080000             ; https://wiki.osdev.org/RSDP
    mov rax, 0x2052545020445352     ; "RSD PTR " in a packed long, the RSDP structure starts with this signature, we can use it to locate the table
.loop:
    cmp rax, [rcx]
    je .exit
    add rcx, 16
    cmp rcx, 0x000FFFFF
    je .exit_not_found
    jmp .loop
.exit_not_found:
    mov rcx, 0
.exit:
    mov rax, rcx   
    ret

enumerate_pci:
    ret
