; stage2.asm
; stage 2 of bootloader. for now, just confirms it loaded and ran correctly

[org 0x7e00]
[bits 16]

jmp stage2_start

%include "boot/a20.asm"

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
