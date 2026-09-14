; stage2.asm
; stage 2 of bootloader. for now, just confirms it loaded and ran correctly

[bits 16]

global stage2_start
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
