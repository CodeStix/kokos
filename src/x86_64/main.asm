; Give other files access to the 'start' address
global start

; Mark the following code to be in the text section, which typically stores code.
section .text

; Tell nasm that 32 bit assembly must be generated below
bits 32

; Name the following block of code 'start', when refernced, start will contain the address of its first instruction (mov)
start:
    mov dword [0xb8000], 0x2f4b2f4f
    hlt