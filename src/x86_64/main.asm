; Give other files access to the 'start' address
global start32
global gdt64.code_segment
global page_table_level4
global memory_map
global multiboot2_info
extern start64

; Mark the following code to be in the text section, which typically stores code.
section .text

; Tell nasm that 32 bit assembly must be generated below
bits 32

; Name the following block of code 'start', when refernced, start will contain the address of its first instruction (mov)
start32:
    mov esp, stack_top      ; Set the address of the stack, so that push and pop instructions will work correctly
    mov [multiboot2_info], ebx
    cli                     ; Disable hardware interrupts

    call check_cpuid_supported
    call check_long_mode

    call paging_map_first_gb
    call enable_pae
    call enable_long_mode
    call enable_paging

    lgdt [gdt64.pointer]    ; Load global descriptor table
    jmp gdt64.code_segment:start64  ; Jump to start64, using the code_segment declared in the global descriptor table

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

check_long_mode:            ; (64 bit mode)
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

paging_map_first_gb:
    mov eax, page_table_level3      ; Store address of level 3 table (page directory pointer table) 
    or eax, 0b11                ; Set present and read/write flag of entry https://wiki.osdev.org/Paging
    mov [page_table_level4], eax    ; Store address of the only level 3 table into first entry of level 4 table (page map level 4 table)

    mov eax, page_table_level2      ; Store address of level 2 table (page directory table)
    or eax, 0b11                ; Set present and read/write flag of entry https://wiki.osdev.org/Paging
    mov [page_table_level3], eax    ; Store address of the only level 2 table into first entry of level 3 table

    mov ecx, 0                  ; Fill the level 2 table with entries, no level 1 table is needed because huge pages are used 
.loop:
    mov eax, 0x200000
    mul ecx 
    or eax, 0b10000011	                    ; Set present, read/write and huge page flag of page (see AMD Volume 2 '2-Mbyte PDE—PAE Paging Legacy-Mode')
    mov [page_table_level2 + ecx * 8], eax      
    inc ecx
    cmp ecx, 512
    jne .loop

    mov eax, page_table_level4      ; Store the address of the 4th level table into cr3
    mov cr3, eax
    ret

enable_pae:
    mov eax, cr4                ; Enable PAE (extended pages, required for long mode, each page table entry will be 64 bit now instead of 32)
    or eax, 1 << 5
    mov cr4, eax
    ret

enable_long_mode:
    mov ecx, 0xC0000080         ; Enable long mode https://wiki.osdev.org/CPU_Registers_x86-64#IA32_EFER
    rdmsr
    or eax, 1 << 8              ; Enable long mode by toggling the 8th bit in the EFER register
    wrmsr
    ret

enable_paging:
    mov eax, cr0                ; Enable paging
    or eax, 1 << 31
    mov cr0, eax
    ret


section .bss ; The bbs section contains uninitialized variables, so the stack too

stack_bottom: ; Reserve 1024 * 16 bytes for the stack
    resb 1024 * 16
stack_top:

align 4096  ; Enforce the next address to be a multiple of 4096 (required for the page tables)

page_table_level4:
    resb 4096   ; Reserve 4096 bytes
page_table_level3:
    resb 4096
page_table_level2:
    resb 4096

multiboot2_info:
    dq 0

section .rodata ; The rodata section contains initialized read only data

; Create a global descriptor table, which we will not use, but is required 
; to enter protected mode (old way of virtual memory using segmentation) https://wiki.osdev.org/Global_Descriptor_Table
gdt64:          
    dq 0                                                ; Empty entry
.code_segment: equ $ - gdt64
    dq (1 << 43) | (1 << 44) | (1 << 47) | (1 << 53)    ; Code segment entry
.pointer:
    dw $ - gdt64 - 1
    dq gdt64
