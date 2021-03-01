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
    mov al, 7         ;sectors to read
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
global Disk_IO
Disk_IO:
    ;  +0xC: [  DAP pointer ]
    ;  +0x8: [  action ]
    ;  +0x4: [  retAddr]
    ;     0: [  ebp  ]
    ;
    push ebp
    mov ebp, esp
    pusha
    ; save all paramters, esp and ebp outside of stack
    mov eax, [ebp+0x8]
    mov [cpy_action], eax
    mov eax, [ebp+0xC]
    mov [cpy_dap_ptr], eax

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
    ; sti         ; Restore interrupts -- be careful, unhandled int's will kill it.
    ; now we are in real mode
    
    ; prepare BIOS call to read from disk
    mov byte [ret_val], 0  ; clear return value
    mov ah, byte [cpy_action]    ; action: 0x02 = read sectors, 0x03 = write sectors
    mov si, [cpy_dap_ptr]
    mov dl, [disk] ;disk idx
    int 0x13
    jc bios_call_err
    jmp GoPMode

bios_call_err:
    mov [ret_val], ah  ; save error code

GoPMode:
    ; cli
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
    ; sti
    ; restore esp and ebp in case they are overwritten
    mov esp, [cpy_esp]
    mov ebp, [cpy_ebp]
    popa
    xor eax, eax
    mov al, byte [ret_val]  ; bring back saved return value
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
boot3:
    cli ; otherwise timer interrupt kills program
    mov esp, kernel_stack_top
    mov ebp, esp

    extern kmain
    call kmain
    jmp halt

global callKernel
callKernel:
    ; +0x10: [  real_mode_and_heap_size ]
    ;  +0xC: [  kernel_entry_seg ]
    ;  +0x8: [  real_mode_code_seg ]
    ;  +0x4: [  retAddr]
    ;     0: [  ebp  ]
    ;
    push ebp
    mov ebp, esp
    ; save all paramters, esp and ebp outside of stack
    mov eax, [ebp+0x8]
    mov [cpy_rm_code_seg], ax
    mov eax, [ebp+0xC]
    mov [cpy_k_entry_seg], ax
    mov eax, [ebp+0x10]
    mov [cpy_rm_and_heap_size], ax

    cli         ; Disable interrupts.
    jmp 0x18:boom2
    ; Need 16-bit Protected Mode GDT entries!
boom2:
bits 16
    mov ax, 0x20  ; 16-bit Protected Mode data selector.
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
 
    mov eax, cr0
    and al, 0xFE ; Disable paging bit & disable 16-bit pmode.
    mov cr0, eax
 
    jmp 0x00:GoRMode2       ; Perform Far jump to set CS.

GoRMode2:
    xor eax, eax
    xor ebx, ebx
    xor ecx, ecx
    mov ax, word [cpy_rm_code_seg]
    mov bx, word [cpy_k_entry_seg]
    mov cx, word [cpy_rm_and_heap_size]
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    mov sp, cx

    push bx
    push 0
    retf
    ;jmp [cpy_k_entry_seg]:0

bits 32
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
cpy_action: resd 1
cpy_dap_ptr: resd 1
ret_val:    resd 1
cpy_rm_and_heap_size: resd 1
cpy_k_entry_seg: resd 1
cpy_rm_code_seg: resd 1
kernel_stack_bottom: equ $
    resb 8192 ; 8 KiB
kernel_stack_top:
global tmp_buffer_ptr
tmp_buffer_ptr: equ $ 
    resb 512
tmp_buffer_ptr_end: