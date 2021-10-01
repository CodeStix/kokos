build: src/x86_64/*.asm
	rm -rf build

	mkdir build build/x86_64
	nasm -f elf64 src/x86_64/main.asm -o build/x86_64/main.o
	nasm -f elf64 src/x86_64/main64.asm -o build/x86_64/main64.o
	nasm -f elf64 src/x86_64/multiboot2.asm -o build/x86_64/multiboot2.o
	x86_64-elf-ld -n -o build/x86_64/os.bin -T linker.ld build/x86_64/*.o

	mkdir build/x86_64/multiboot2 build/x86_64/multiboot2/boot build/x86_64/multiboot2/boot/grub
	cp grub.cfg build/x86_64/multiboot2/boot/grub
	cp build/x86_64/os.bin build/x86_64/multiboot2/boot/os.bin
	grub-mkrescue /usr/lib/grub/i386-pc -o build/x86_64/build.iso build/x86_64/multiboot2

run:
	qemu-system-x86_64 build/x86_64/build.iso