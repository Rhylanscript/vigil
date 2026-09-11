; boot.asm
; stage 1 bootloader thats runs stage 2 from disk, then jumps to it

[org 0x7c00]
[bits 16]

start:
    mov [boot_drive], dl

    mov si, msg_loading
    call print_string

    call load_stage2
    jmp 0x0000:0x7e00

print_string:
    lodsb
    or al, al
    jz .done
    mov ah, 0x0e
    int 0x10
    jmp print_string
.done:
    ret

load_stage2:
    mov ah, 0x02
    mov al, 4
    mov ch, 0
    mov cl, 2
    mov dh, 0
    mov dl, [boot_drive]
    mov bx, 0x7e00

    int 0x13
    jc disk_error

    ret

disk_error:
    mov si, msg_disk_error
    call print_string
    jmp $

boot_drive db 0
msg_loading db 'VIGIL: Loading...', 13, 10, 0
msg_disk_error db 'VIGIL: disk read error', 13, 10, 0

times 510-($-$$) db 0
dw 0xaa55
