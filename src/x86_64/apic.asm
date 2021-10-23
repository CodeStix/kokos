global apic_disable_pic
global apic_check_supported

section .text
bits 64

; TODO check if APIC is enabled in MSR

; Because we will use the newer APIC (advanced programmable interrupt controller) instead of the old PICs
; we need to disable the old PIC. Because PIC's can generate random interrupts (by electric noise ect), we must also offset these interrupts to an unused
; region of interrupt vectors, so it does not trigger random system interrupts (worse: exception interrupts)
; apic_disable_pic:
;     mov al, 0x11            ; Start master PIC initialization sequence, it now expects 3 bytes at port 0x21 to configure this PIC
;     out 0x20, al          

;     mov al, 0               ; Port 80 is unused by the computer and is used to simulate a delay, because it could take some time before the PIC answers
;     out 0x80, al             

;     mov al, 0xFF - 16
;     out 0x21, al            ; Set master interrupt vectors to be 0xFF - 16 -> 0xFF - 8

;     mov al, 0
;     out 0x80, al
    
;     mov al,  0b00000100     ; Tell master PIC that there is a slave PIC at IRQ2 pin
;     out 0x21, al            

;     mov al, 0
;     out 0x80, al

;     mov al, 0x01            ; PIC mode 1 (?)
;     out 0x21, al          

;     mov al, 0
;     out 0x80, al


;     mov al, 0x11            ; Start slave PIC initialization sequence, it now expects 3 bytes at port 0x21 to configure this PIC
;     out 0xa0, al          

;     mov al, 0
;     out 0x80, al

;     mov al, 0xFF - 8        ; Set slave interrupt vectors to be 0xFF - 8 -> 0xFF (each PIC has 8 interrupts)
;     out 0xa1, al      

;     mov al, 0
;     out 0x80, al

;     mov al, 0b00000010      ; Tell slave PIC that it is a slave
;     out 0xa1, al    

;     mov al, 0
;     out 0x80, al

;     mov al, 0x01            ; PIC mode 1 (?) 
;     out 0xa1, al          

;     mov al, 0
;     out 0x80, al


;     mov al, 0xff
;     out 0xa1, al            ; Disable slave pic (by masking all interrupts using 0xff)
;     out 0x21, al            ; Disable master pic

;     ret

; apic_check_supported:
;     push rbx    
;     mov rax, 0x1            ; Execute function 1 of cpuid, it must be stored in rax
;     cpuid
;     pop rbx
;     test rdx, 0b1_00000000  ; Test for bit 9 in the rdx register, which means that an apic is supported (amd64 vol3 p574)
;     jz .not_supported
;     mov rax, 1
;     ret
; .not_supported:
;     mov rax, 0
;     ret

    
