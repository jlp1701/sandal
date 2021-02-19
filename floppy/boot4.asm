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
    mov al, 6      ;sectors to read
    mov ch, 0      ;cylinder idx
    mov dh, 0      ;head idx
    mov cl, 2      ;sector idx
    mov dl, [disk] ;disk idx
    mov bx, copy_target;target pointer
    int 0x13
    
    lgdt [gdt_pointer] ; load the gdt table
RM2PM:
    mov eax, cr0 
    or eax,0x1 ; set the protected mode bit on special CPU reg cr0
    mov cr0, eax

    ; initializing the segment register
    mov ax, DATA_SEG_32
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    jmp CODE_SEG_32:boot3  ; long jump to the code segment

 
idt_real:
    dw 0x3ff        ; 256 entries, 4b each = 1K
    dd 0            ; Real Mode IVT @ 0x0000
 
savcr0:
    dd 0            ; Storage location for pmode CR0.

bits 32
Entry16:
    ; We are already in 16-bit mode here!
 
    cli         ; Disable interrupts.
 
    ; Need 16-bit Protected Mode GDT entries!
    mov eax, DATA_SEG_16  ; 16-bit Protected Mode data selector.
    mov ds, eax
    mov es, eax
    mov fs, eax
    mov gs, eax
    mov ss, eax
 
 
    ; Disable paging (we need everything to be 1:1 mapped).
    mov eax, cr0
    ;mov [savcr0], eax   ; save pmode CR0
    and eax, 0x7FFFFFFe ; Disable paging bit & disable 16-bit pmode.
    mov cr0, eax
 
    jmp CODE_SEG_16:GoRMode       ; Perform Far jump to set CS.

bits 16
GoRMode:
    ;mov sp, 0x8000      ; pick a stack pointer.
    mov ax, 0       ; Reset segment registers to 0.
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    lidt [idt_real]
    sti         ; Restore interrupts -- be careful, unhandled int's will kill it.
    call RM2PM
    ret

disk: db 0

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
    ;  Limit: 0xFFFFF
    ; Base: 0x0
    ; 16-bit
    ; Privilege level: 0
    ; Granularity: 0
    ; Read and Write: 1 
    dw 0xFFFF   ; Limit 0:15
    dw 0x0      ; Base  0:15
    db 0x0      ; Base  32:39
    db 10011010b  ; Access Byte
    db 00001111b  ; Flags, Limit 16:19
    db 0x0        ; Base 24:31
gdt_data_16:
    dw 0xFFFF   ; Limit 0:15
    dw 0x0      ; Base  0:15
    db 0x0      ; Base  32:39
    db 10010010b  ; Access Byte
    db 00001111b  ; Flags, Limit 16:19
    db 0x0        ; Base 24:31
gdt_end:

gdt_pointer:
    dw gdt_end - gdt_start
    dd gdt_start
CODE_SEG_32 equ gdt_code_32 - gdt_start
DATA_SEG_32 equ gdt_data_32 - gdt_start
CODE_SEG_16 equ gdt_code_16 - gdt_start
DATA_SEG_16 equ gdt_data_16 - gdt_start


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
    call Entry16  ; test: change right back to real mode
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
times 1024 - ($-$$) db 0

section .bss
align 4
kernel_stack_bottom: equ $
    resb 16384 ; 16 KiB
kernel_stack_top:

section .data