; stage2.asm
; stage 2 of bootloader. for now, just confirms it loaded and ran correctly

[org 0x7e00]
[bits 16]

jmp stage2_start

%include "boot/a20.asm"
%include "boot/gdt.asm"

stage2_start:
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

    mov edi, 0xb8000

    mov al, 'V'
    mov ah, 0x0f
    mov [edi], ax
    mov al, 'I'
    mov [edi+2], ax
    mov al, 'G'
    mov [edi+4], ax
    mov al, 'I'
    mov [edi+6], ax
    mov al, 'L'
    mov [edi+8], ax

    jmp $

print_string:
    lodsb
    or al, al
    jz .done
    mov ah, 0x0e
    int 0x10
    jmp print_string
.done:
    ret

msg_stage2 db 'VIGIL: stage 2 online', 13, 10, 0
msg_a20_ok db 'VIGIL: A20 enabled', 13, 10, 0
msg_a20_fail db 'VIGIL: A20 FAILED', 13, 10, 0

times 2048-($-$$) db 0
; pad Stage 2 out to exactly 2048 bytes as this needs to match the sector
; count boot.asm asks the BIOS to read
