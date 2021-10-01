; This file contains the header required to work with bootloaders that support the multiboot2 spec
; https://www.gnu.org/software/grub/manual/multiboot2/multiboot.html#Specification

section .multiboot2_header

header_start:

    ; https://www.gnu.org/software/grub/manual/multiboot2/multiboot.html#Header-magic-fields
    ; magic number for multiboot2, so that the bootloader can recognize that this os supports multiboot2
    dd 0xe85250d6

    ; architecture: protected mode i386
    dd 0

    ; header length
    dd header_end - header_start

    ; create checksum
    dd 0x100000000 - (0xe85250d6 + 0 + (header_end - header_start))

    ; https://www.gnu.org/software/grub/manual/multiboot2/multiboot.html#Header-tags
    ; ending tag
    dw 0
    dw 0
    dd 8

header_end: