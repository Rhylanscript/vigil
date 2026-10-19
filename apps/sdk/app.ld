/* apps/sdk/app.ld
   linker script for loadable VIGIL apps
   Build with:  ld -T app.ld -Ttext=<slot address> ...  */

ENTRY(app_entry)

SECTIONS
{
    .text : {
        __app_start = .;

        /* the 24 byte vapp header, written by linker */
        LONG(0x50504156)                    /* magic: bytes V A P P         */
        LONG(1)                             /* header format version        */
        LONG(__app_start)                   /* address app was linked at    */
        LONG(__image_end - __app_start)     /* bytes stored in the file     */
        LONG(__memory_end - __app_start)    /* bytes needed in ram          */
        LONG(app_entry - __app_start)       /* where app entry lives        */

        *(.text .text.*)
    }

    .rodata : { *(.rodata .rodata.*) }

    .data : {
        *(.data .data.*)
        . = ALIGN(4);
    }

    __image_end = .;

    .bss : {
        *(.bss .bss.*)
        *(COMMON)
        . = ALIGN(4);
    }

    __memory_end = .;

    /DISCARD/ : { *(.eh_frame) *(.comment) *(.note*) }
}
