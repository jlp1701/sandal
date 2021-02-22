section .text ; do not change name of section: breaks gdb debugging symbols


bits 16
; org 0x7c00
global boot
boot:
    mov ax, 0x2401
    int 0x15 ; enable A20 bit

    mov ax, 0x3
    int 0x10 ; set vga text mode 3
    
    mov [disk], dl  ; save disk number
    
    ; load second sector into memory
    mov ah, 0x2    ;read sectors
    mov al, 6         ;sectors to read
    mov ch, 0      ;cylinder idx
    mov dh, 0      ;head idx
    mov cl, 2      ;sector idx
    mov dl, [disk] ;disk idx
    mov bx, copy_target;target pointer
    int 0x13
    
    lgdt [gdt_pointer] ; load the gdt table
    sgdt [gdtp]
    mov eax, cr0 
    or eax,0x1 ; set the protected mode bit on special CPU reg cr0
    mov cr0, eax

    ; initializing the segment register
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    jmp 0x8:boot3  ; long jump to the code segment

; RM2PM:
;     cli
;     ; Restore GDT
;     lgdt [ss:gdtp]
;     ; lgdt [gdt_pointer] ; load the gdt table

;     mov eax, cr0 
;     or al, 0x1 ; set the protected mode bit on special CPU reg cr0
;     mov cr0, eax

;     ; initializing the segment register
;     mov ax, 0x10
;     mov ds, ax
;     mov es, ax
;     mov fs, ax
;     mov gs, ax
;     mov ss, ax
;     jmp 0x8:tjmp
; bits 32
; tjmp:
;     sti
;     ret

bits 32
global ReadSectorFromDisk  ; (unsigned long numToRead, unsigned long head, unsigned long sector, unsigned long cylinder, unsigned int targetAddr)
ReadSectorFromDisk:
    ; +0x18: [  targetAddr ]
    ; +0x14: [  cylinder ]
    ; +0x10: [  sector ]
    ;  +0xC: [  head ]
    ;  +0x8: [  numToRead]
    ;  +0x4: [  retAddr]
    ;     0: [  ebp  ]
    ;
    push ebp
    mov ebp, esp
    ; save all paramters, esp and ebp outside of stack
    mov eax, [ebp+0x8]
    mov [cpy_numToRead], eax
    mov eax, [ebp+0xC]
    mov [cpy_head], eax
    mov eax, [ebp+0x10]
    mov [cpy_sector], eax
    mov eax, [ebp+0x14]
    mov [cpy_cylinder], eax
    mov eax, [ebp+0x18]
    mov [cpy_targetAddr], eax
    mov [cpy_esp], esp
    mov [cpy_ebp], ebp

    ; Save GDT in case BIOS overwrites it
    sgdt [gdtp]
    cli         ; Disable interrupts.

    jmp 0x18:boom
    ; Need 16-bit Protected Mode GDT entries!
boom:
bits 16
    mov ax, 0x20  ; 16-bit Protected Mode data selector.
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
 
 
    ; Disable paging (we need everything to be 1:1 mapped).
    mov eax, cr0
    ;mov [savcr0], eax   ; save pmode CR0
    and al, 0xFE ; Disable paging bit & disable 16-bit pmode.
    mov cr0, eax
 
    jmp 0x00:GoRMode       ; Perform Far jump to set CS.

GoRMode:
    ;mov sp, 0x8000      ; pick a stack pointer.
    mov ax, 0       ; Reset segment registers to 0.
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    ; lidt [idt_real]
    sti         ; Restore interrupts -- be careful, unhandled int's will kill it.
    ; now we are in real mode
    
    ; load second sector into memory
    mov ah, 0x2    ;read sectors
    mov al, byte [cpy_numToRead]      ;sectors to read
    mov ch, byte [cpy_cylinder]      ;cylinder idx
    mov dh, byte [cpy_head]      ;head idx
    mov cl, byte [cpy_sector]      ;sector idx
    mov dl, [disk] ;disk idx
    mov bx, word [cpy_targetAddr];target pointer
    int 0x13

    cli
    ; Restore GDT
    lgdt [ss:gdtp]

    mov eax, cr0 
    or al, 0x1 ; set the protected mode bit on special CPU reg cr0
    mov cr0, eax

    ; initializing the segment register
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    jmp 0x8:tjmp
bits 32
tjmp:
    sti
    ; restore esp and ebp in case they are overwritten
    mov esp, [cpy_esp]
    mov ebp, [cpy_ebp]
    leave
    ret

align 4
gdt_start:
    dq 0x0
gdt_code_32:
    dw 0xFFFF
    dw 0x0
    db 0x0
    db 10011010b
    db 11001111b
    db 0x0
gdt_data_32:
    dw 0xFFFF
    dw 0x0
    db 0x0
    db 10010010b
    db 11001111b
    db 0x0
gdt_code_16:
    dw 0xFFFF
    dw 0x0
    db 0x0
    db 10011010b
    db 00001111b
    db 0x0
gdt_data_16:
    dw 0xFFFF
    dw 0x0
    db 0x0
    db 10010010b
    db 00001111b
    db 0x0
gdt_end:

gdt_pointer:
    dw gdt_end - gdt_start
    dd gdt_start
CODE_SEG_32 equ gdt_code_32 - gdt_start
DATA_SEG_32 equ gdt_data_32 - gdt_start


times 510 - ($-$$) db 0 ; pad remaining 510 bytes with zeroes
dw 0xaa55 ; magic bootloader magic - marks this 512 byte sector bootable!
    
bits 32
align 4
copy_target:
    ; hello: db "Hello more than 512 bytes world!!",0
    hello: db "ABC",0
align 4
boot3:
    mov esp, kernel_stack_top
    mov ebp, esp
    ;call ReadSectorFromDisk  ; test: change right back to real mode
    ; call RM2PM
    mov esi, hello
    xor ebx, ebx
    mov ebx, 0xb8000
.loop:
    lodsb
    or al,al
    jz cpp
    or eax,0x0F00
    mov word [ebx], ax
    add ebx,2
    jmp .loop

cpp:
    extern kmain
    call kmain
    jmp halt

global myfunc
myfunc:
    push ebp
    mov ebp, esp
    xor eax, eax
    mov al, byte [ebp+8]
    add al, byte [ebp+12]
    leave
    ret

halt:
    cli
    hlt
;times 1024 - ($-$$) db 0

section .bss
align 16
disk: resd 1
gdtp: resb 8
; space for ReadSectorFromDisk parameter when in RealMode
cpy_esp: resd 1
cpy_ebp: resd 1
cpy_numToRead: resd 1
cpy_head: resd 1
cpy_sector: resd 1
cpy_cylinder: resd 1
cpy_targetAddr: resd 1
kernel_stack_bottom: equ $
    resb 8192 ; 8 KiB
kernel_stack_top:

section .data