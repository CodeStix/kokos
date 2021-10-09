global pci_config_read32
global port_out32
global port_in32
global port_out8
global port_in8

section .text
bits 64

; https://wiki.osdev.org/PCI
; rdi contains bus number
; rsi contains slot number
; rdx contains function number
; rcx contains which part of the pci register to read (must be a multiple of 4)
pci_config_read32:               
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
    or rax, rcx

    mov edx, 0xCF8
    out dx, eax         ; Send which part of pci configuration space to read

    mov eax, 0
    mov edx, 0xCFC
    in eax, dx            ; Read pci configuration space

    ret

port_out32:
    mov rdx, rdi
    mov rax, rsi
    out dx, eax
    ret

port_in32:
    mov eax, 0
    mov rdx, rdi
    in eax, dx 
    ret           

port_out8:
    mov rdx, rdi
    mov rax, rsi
    out dx, al
    ret

port_in8:
    mov eax, 0
    mov rdx, rdi
    in al, dx 
    ret           