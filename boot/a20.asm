; a20.asm

enable_a20:
    in al, 0x92
    or al, 2
    out 0x92, al
    ret

check_a20:
    push es
    push ds
    push di
    push si

    xor ax, ax
    mov es, ax
    mov di, 0x0500

    mov ax, 0xffff
    mov ds, ax
    mov si, 0x0510

    mov byte [es:di], 0x00
    mov byte [ds:si], 0xff

    cmp byte [es:di], 0xff

    pop si
    pop di
    pop ds
    pop es

    je .disabled
    mov al, 1
    ret
.disabled:
    xor al, al
    ret
