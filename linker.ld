ENTRY(start)

SECTIONS 
{
    /* reserve 1 megabyte of unused space */
    . = 1M;

    /* create the boot section, which contains our multiboot2 header */
    .boot : 
    {
        /* KEEP: output even if nothing in multiboot2_header is referenced.
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
}