; boot/stage2.asm
; stage 2 of bootloader.

[bits 16]

global stage2_start
jmp stage2_start

%include "boot/a20.asm"
%include "boot/gdt.asm"

stage2_start:
    cli
    xor ax, ax
    mov ss, ax
    mov sp, 0x7c00
    sti

    mov si, msg_stage2
    call print_string

    call enable_a20
    call check_a20
    cmp al, 1
    je .a20_ok

    mov si, msg_a20_fail
    call print_string
    jmp $

.a20_ok:
    mov si, msg_a20_ok
    call print_string

    mov ax, 0x2000
    mov es, ax
    xor di, di
    ; es:di = 0x2000:0x0000 = physical address 0x20000

    mov ax, 0x4f01
    mov cx, VBE_TEST_MODE
    int 0x10
    ; bios function 0x4F01 = vbe get mode info - asks for details
    ; about specific mode num in cx, writing result into a ~256 byte 
    ; structure at es:di

    cmp ax, 0x004f
    jne .vbe_unsupported

    mov si, msg_vbe_found
    call print_string

    mov ax, [es:di + 0x12]
    call print_hex16
    mov si, msg_x
    call print_string
    mov ax, [es:di + 0x14]
    call print_hex16
    mov si, msg_at
    call print_string
    mov al, [es:di + 0x19]
    mov ah, 0
    call print_hex16
    mov si, msg_bpp
    call print_string
    ; pull xresolution (offset 0x12), yresolution (offset 0x14), and
    ; bitsperpixel (offset 0x19) out of returned structure

    ; mov si, msg_press_key
    ; call print_string
    ; xor ax, ax
    ; int 0x16
    ; pauses boot process until user presses a key

    jmp .vbe_done

.vbe_unsupported:
    mov si, msg_vbe_fail
    call print_string
    jmp $

.vbe_done:
    cli

    lgdt [gdt_descriptor]

    mov eax, cr0
    or eax, 1
    mov cr0, eax

    jmp CODE_SEG:protected_mode_start

[bits 32]

protected_mode_start:
    mov ax, DATA_SEG
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    mov esp, 0x90000

    mov edi, 0xb8000
    mov ecx, 80 * 25

.clear_loop:
    mov word [edi], 0x0f20
    add edi, 2
    loop .clear_loop

    extern kernel_main
    extern __bss_start
    extern __bss_end

    mov edi, __bss_start
    mov ecx, __bss_end
    sub ecx, edi
    
    xor eax, eax
    rep stosb
    ; repeats store al at [edi] and increment edi ecx times

    call kernel_main

    jmp $
    ; safety net - kernel_main shouldnt return but if it does
    ; stop here

[bits 16]

print_string:
    lodsb
    or al, al
    jz .done
    mov ah, 0x0e
    int 0x10
    jmp print_string
.done:
    ret

print_hex16:
    push ax
    push bx
    push cx
    push si
    push di

    mov di, hex_buffer
    mov cx, 4
.hex_loop:
    rol ax, 4
    mov bx, ax
    and bx, 0x000f
    mov bl, [hex_chars + bx]
    mov [di], bl
    inc di
    loop .hex_loop

    mov byte [di], 0

    pop di
    pop si
    pop cx
    pop bx
    pop ax

    push si
    mov si, hex_buffer
    call print_string

    pop si
    ret

hex_chars db '0123456789ABCDEF'
hex_buffer: times 5 db 0

msg_stage2 db 'VIGIL: stage 2 online', 13, 10, 0

msg_a20_ok db 'VIGIL: A20 enabled', 13, 10, 0
msg_a20_fail db 'VIGIL: A20 FAILED', 13, 10, 0

msg_vbe_found db 'VIGIL: VBE mode found - ', 0
msg_vbe_fail db 'VIGIL: VBE mode unavailable', 13, 10, 0

msg_x db 'x', 0
msg_at db ' @ ', 0              ; why does it say this is a comment bro the @ is literally in comments
msg_bpp db 'bpp', 13, 10, 0
msg_press_key db 'VIGIL: press any key to continue...', 13, 10, 0

VBE_TEST_MODE equ 0x118
