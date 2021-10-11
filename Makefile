# make build && cp build/x86_64/build.iso /mnt/c/Users/Stijn/Downloads/build.iso

build: src/x86_64/** src/common/** src/include/**
	rm -rf build

	mkdir build build/x86_64 build/common
	nasm -f elf64 src/x86_64/main.asm -o build/x86_64/main.o
	nasm -f elf64 src/x86_64/main64.asm -o build/x86_64/main64.o
	nasm -f elf64 src/x86_64/multiboot2.asm -o build/x86_64/multiboot2.o
	nasm -f elf64 src/x86_64/apic.asm -o build/x86_64/apic.o
	x86_64-elf-gcc -c -I src/include -ffreestanding src/common/console.c -o build/common/console.o
	x86_64-elf-gcc -c -I src/include -ffreestanding src/common/acpi.c -o build/common/acpi.o
	x86_64-elf-gcc -c -I src/include -ffreestanding src/common/pci.c -o build/common/pci.o
	x86_64-elf-gcc -c -I src/include -ffreestanding src/common/memory.c -o build/common/memory.o
	x86_64-elf-gcc -c -I src/include -ffreestanding src/common/util.c -o build/common/util.o
	x86_64-elf-gcc -c -I src/include -ffreestanding src/common/keyboard.c -o build/common/keyboard.o
	x86_64-elf-gcc -c -I src/include -ffreestanding src/common/apic.c -o build/common/apic.o
	x86_64-elf-gcc -c -I src/include -ffreestanding src/common/main.c -o build/common/main.o
	x86_64-elf-ld -n -o build/x86_64/os.bin -T linker.ld build/x86_64/*.o build/common/*.o  

	mkdir build/x86_64/multiboot2 build/x86_64/multiboot2/boot build/x86_64/multiboot2/boot/grub
	cp grub.cfg build/x86_64/multiboot2/boot/grub
	cp build/x86_64/os.bin build/x86_64/multiboot2/boot/os.bin
	grub-mkrescue /usr/lib/grub/i386-pc -o build/x86_64/build.iso build/x86_64/multiboot2

run:
	qemu-system-x86_64 build/x86_64/build.iso