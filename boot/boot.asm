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
    mov si, dap
    mov ah, 0x42

    mov dl, [boot_drive]
    int 0x13

    jc disk_error
    ret

dap:
    db 0x10
    ; size of packet in bytes, always 16 (0x10)
    db 0
    ; reserved by bios spec (always 0)
    dw 64
    ; number of sectors to read. 64 sectors = 32kb
    dw 0x7e00
    ; offset of where to load data in memory
    dw 0x0000
    ; segment of where to load data in memory. With above, it points
    ; to address 0x7e00 
    dq 1
    ; starting LBA to read from. as LBA numbering starts at 0, Stage
    ; 2 begins at LBA 1, after boot sector which is 0

disk_error:
    mov si, msg_disk_error
    call print_string
    jmp $

boot_drive db 0
msg_loading db 'VIGIL: Loading...', 13, 10, 0
msg_disk_error db 'VIGIL: disk read error', 13, 10, 0

times 510-($-$$) db 0
dw 0xaa55
