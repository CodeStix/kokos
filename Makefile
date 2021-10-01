

build:
		mkdir build build/x86_64
		nasm -f elf64 src/x86_64/main.asm -o build/x86_64/main.o
		nasm -f elf64 src/x86_64/multiboot2.asm -o build/x86_64/multiboot2.o
		x86_64-elf-ld -n -o build/x86_64/os.bin -T linker.ld build/x86_64/*.o

		cp build/x86_64/os.bin multiboot2/boot/os.bin
		grub-mkrescue /usr/lib/grub/i386-pc -o build.iso multiboot2/