; boot/isr.asm

%macro ISR_NOERRCODE 1
global isr%1
isr%1:
    cli
    push byte 0
    push byte %1
    jmp isr_common_stub
%endmacro

%macro ISR_ERRCODE 1
global isr%1
isr%1:
    cli
    push byte %1
    jmp isr_common_stub
%endmacro

ISR_NOERRCODE 0   ; divide by zero
ISR_NOERRCODE 1   ; debug
ISR_NOERRCODE 2   ; non maskable interrupt
ISR_NOERRCODE 3   ; breakpoint
ISR_NOERRCODE 4   ; overflow
ISR_NOERRCODE 5   ; bound range exceeded
ISR_NOERRCODE 6   ; invalid opcode
ISR_NOERRCODE 7   ; device not available
ISR_ERRCODE   8   ; double fault
ISR_NOERRCODE 9   ; coprocessor segment overrun (legacy)
ISR_ERRCODE   10  ; invalid tss
ISR_ERRCODE   11  ; segment not present
ISR_ERRCODE   12  ; stack segment fault
ISR_ERRCODE   13  ; general protection fault
ISR_ERRCODE   14  ; page fault
ISR_NOERRCODE 15  ; eeserved
ISR_NOERRCODE 16  ; x87 floatin point exception
ISR_ERRCODE   17  ; alignment check
ISR_NOERRCODE 18  ; machine check
ISR_NOERRCODE 19  ; SIMD floating point exception
ISR_NOERRCODE 20  ; virtualization exception
ISR_NOERRCODE 21
ISR_NOERRCODE 22
ISR_NOERRCODE 23
ISR_NOERRCODE 24
ISR_NOERRCODE 25
ISR_NOERRCODE 26
ISR_NOERRCODE 27
ISR_NOERRCODE 28
ISR_NOERRCODE 29
ISR_NOERRCODE 30
ISR_NOERRCODE 31

extern isr_handler

isr_common_stub:
    pusha

    mov ax, ds
    push eax

    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    call isr_handler

    pop eax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    popa

    add esp, 8
    
    sti
    iret
