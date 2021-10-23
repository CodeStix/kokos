# make build && cp build/x86_64/build.iso /mnt/c/Users/Stijn/Downloads/build.iso

build: src/x86_64/** src/common/** src/include/**
	rm -rf build

	mkdir build build/x86_64 build/common
	nasm -f elf64 src/x86_64/main.asm -o build/x86_64/main.o
	nasm -f elf64 src/x86_64/main64.asm -o build/x86_64/main64.o
	nasm -f elf64 src/x86_64/multiboot2.asm -o build/x86_64/multiboot2.o
	nasm -f elf64 src/x86_64/apic.asm -o build/x86_64/apic.o
	gcc -c -I src/include -masm=intel -nostdlib -ffreestanding -mno-red-zone -fno-stack-protector src/common/console.c -o build/common/console.o
	gcc -c -I src/include -masm=intel -nostdlib -ffreestanding -mno-red-zone -fno-stack-protector src/common/acpi.c -o build/common/acpi.o
	gcc -c -I src/include -masm=intel -nostdlib -ffreestanding -mno-red-zone -fno-stack-protector src/common/pci.c -o build/common/pci.o
	gcc -c -I src/include -masm=intel -nostdlib -ffreestanding -mno-red-zone -fno-stack-protector src/common/memory.c -o build/common/memory.o
	gcc -c -I src/include -masm=intel -nostdlib -ffreestanding -mno-red-zone -fno-stack-protector src/common/util.c -o build/common/util.o
	gcc -c -I src/include -masm=intel -nostdlib -ffreestanding -mno-red-zone -fno-stack-protector src/common/keyboard.c -o build/common/keyboard.o
	gcc -c -I src/include -masm=intel -nostdlib -ffreestanding -mno-red-zone -fno-stack-protector src/common/apic.c -o build/common/apic.o
	gcc -c -I src/include -masm=intel -nostdlib -ffreestanding -mno-red-zone -fno-stack-protector src/common/paging.c -o build/common/paging.o
	gcc -c -I src/include -masm=intel -nostdlib -ffreestanding -mno-red-zone -fno-stack-protector src/common/multiboot2.c -o build/common/multiboot2.o
	gcc -c -I src/include -masm=intel -nostdlib -ffreestanding -mno-red-zone -fno-stack-protector src/common/memory_physical.c -o build/common/memory_physical.o
	gcc -c -I src/include -masm=intel -nostdlib -ffreestanding -mno-red-zone -fno-stack-protector src/common/port.c -o build/common/port.o
	gcc -c -I src/include -masm=intel -nostdlib -ffreestanding -mno-red-zone -fno-stack-protector src/common/cpu.c -o build/common/cpu.o
# Compile using -mgeneral-regs-only so we can use gcc's interrupt attribute (see interrupt.c), this is needed because gcc only preserves general purpose registers and not
# SEE, MMX and x87 registers and states
	gcc -c -I src/include -masm=intel -nostdlib -ffreestanding -mno-red-zone -fno-stack-protector -mgeneral-regs-only src/common/interrupt.c -o build/common/interrupt.o
	gcc -c -I src/include -masm=intel -nostdlib -ffreestanding -mno-red-zone -fno-stack-protector -mgeneral-regs-only src/common/main.c -o build/common/main.o
	ld -n -o build/x86_64/os.bin -T linker.ld build/x86_64/*.o build/common/*.o  

	mkdir build/x86_64/multiboot2 build/x86_64/multiboot2/boot build/x86_64/multiboot2/boot/grub
	cp grub.cfg build/x86_64/multiboot2/boot/grub
	cp build/x86_64/os.bin build/x86_64/multiboot2/boot/os.bin
	grub-mkrescue /usr/lib/grub/i386-pc -o build/x86_64/build.iso build/x86_64/multiboot2

run: build
	qemu-system-x86_64 -usb -m 1G build/x86_64/build.iso