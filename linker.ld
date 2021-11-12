ENTRY(start32)

SECTIONS 
{
    /* set current writing address to 0x8000 (around 8kb) */
    . = 0x8000;

    .lowtext : 
    {
        *(.lowtext)
    }

    /* set current writing address to 0x00100000 (around 1mb) */
    . = 0x00100000;

    /* create the boot section, which contains our multiboot2 header */
    .boot : 
    {
        /*  KEEP: output even if nothing in multiboot2_header is referenced.
            This is needed because a linker links code and removes code that will not be used. Our
            multiboot2_header does not contain any code and thus will not be referenced, but we 
            want to include it in the binary. */
        KEEP(*(.multiboot2_header))
    }

    /* create the text section, which will contain our code */
    .text :
    {
        *(.text)
    }

    .bss :
    {
        *(.bss)
    }

    .data :
    {
        *(.data)
    }

    .rodata :
    {
        *(.rodata)
    }
}