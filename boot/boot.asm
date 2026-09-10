; boot.asm
; minimal x86 bootloader

[org 0x7c00]

start:
    mov si, message
    call print_string
    jmp $

print_string:
    lodsb
    or al, al

    jz done
    mov ah, 0x0e
    int 0x10

    jmp print_string
done:
    ret

message db 'SYSTEM ONLINE...', 0

times 510-($-$$) db 0

dw 0xaa55
