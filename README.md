# OS

## Dependencies

Required for development.

```
sudo apt install build-essential nasm grub-pc-bin grub-common xorriso
```

Required for building cross-compiler.

```
sudo apt-get install -y wget gcc libgmp3-dev libmpfr-dev libisl-dev libcloog-isl-dev libmpc-dev texinfo bison flex make bzip2 patch build-essential
```

You can build the cross compiler using the `build_cross_compiler.sh` script. This compiler is required to build the operating system.

## Notes

-   https://en.wikipedia.org/wiki/Input/output_base_address
-   https://bochs.sourceforge.io/techspec/PORTS.LST
-   https://github.com/qemu/qemu/blob/master/docs/specs/standard-vga.txt
-   https://stackoverflow.com/questions/22280281/on-x86-when-the-os-disables-interrupts-do-they-vanish-or-do-they-queue-and-w
-   https://stackoverflow.com/questions/10679001/cli-before-start-in-bootloader
-   https://stackoverflow.com/questions/45206171/how-sci-system-control-interrupt-vector-is-defined
-   https://stackoverflow.com/questions/57704146/how-to-figure-out-the-interrupt-source-on-i-o-apic
-   https://forum.osdev.org/viewtopic.php?p=282061
-   Dump 16 bit code from secion .lowtext: `objdump -M intel,i8086 -d build/x86_64/os.bin -j .lowtext | less`
-   Dump 32 bit code: `objdump -M intel,i386 -d build/x86_64/os.bin | less`
-   Dump 64 bit code: `objdump -M intel,x86_64 -d build/x86_64/os.bin | less`

## Things I learned/noticed

-   The `hlt` instruction only stops execution until an interrupt is fired. It continues where it left off after the interrupt. It places the processor in a paused state.
-   A Global System Interrupt or GSI is the same as an IRQ.
-   Local processor APIC's ids can be 8 bit instead of 4 bit in some cases (tested on hardware), all the manuals state that the upper 4 bits of the APIC id register are reserved, maybe I only found old manuals. You can assume that the APIC id register and the IO APIC physical destination register are 8 bits instead of 4 bits. (Tested on 4 different systems with different age)
