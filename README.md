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
