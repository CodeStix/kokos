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




; The following function checks if at least APCI 2.0 is supported and checks its checksum
check_rsdp:
    mov rax, 0                  ; Check RSDP 1.0 checksum, the sum of all the bytes of the structure must have its lowest byte equal to 0
    mov rcx, 0
.loop1:
    add rax, [rdi + rcx]
    inc rcx
    cmp rcx, 20
    jne .loop1
    cmp al, 0
    je .ok_checksum1
    mov rax, 1
    ret
.ok_checksum1:
    mov byte rax, [rdi + 15]    ; Check RDSP version (must be at least 2)
    cmp rax, 2
    jnb .ok_version
    mov rax, 2
    ret
.ok_version:
    mov rax, 0                  ; Check RSDP 2.0 checksum, the sum of all the bytes of the structure must have its lowest byte equal to 0
    mov rcx, 0
.loop2:
    add rax, [rdi + rcx]
    inc rcx
    cmp rcx, 36
    jne .loop2
    cmp al, 0
    je .ok_checksum2
    mov rax, 3
    ret
.ok_checksum2:
    mov rax, 0
    ret 





enumerate_pci:
    ret

section .rodata

error_invalid_rdsp:
    db "halt: rdsp has wrong checksum!", 0xA, 0x0