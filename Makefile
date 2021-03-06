# make build && cp build/x86_64/build.iso /mnt/c/Users/Stijn/Downloads/build.iso

build: src/x86_64/** src/common/** src/include/**
	rm -rf build

	mkdir build build/x86_64 build/common
	nasm -f elf64 src/x86_64/main.asm -o build/x86_64/main.o
	nasm -f elf64 src/x86_64/main64.asm -o build/x86_64/main64.o
	nasm -f elf64 src/x86_64/cpu.asm -o build/x86_64/cpu.o
	nasm -f elf64 src/x86_64/multiboot2.asm -o build/x86_64/multiboot2.o
	nasm -f elf64 src/x86_64/schedule.asm -o build/x86_64/schedule.o
	x86_64-elf-gcc -c -I src/include -masm=intel -nostdlib -ffreestanding -mno-red-zone -fno-stack-protector src/common/console.c -o build/common/console.o
	x86_64-elf-gcc -c -I src/include -masm=intel -nostdlib -ffreestanding -mno-red-zone -fno-stack-protector src/common/acpi.c -o build/common/acpi.o
	x86_64-elf-gcc -c -I src/include -masm=intel -nostdlib -ffreestanding -mno-red-zone -fno-stack-protector src/common/pci.c -o build/common/pci.o
	x86_64-elf-gcc -c -I src/include -masm=intel -nostdlib -ffreestanding -mno-red-zone -fno-stack-protector src/common/memory.c -o build/common/memory.o
	x86_64-elf-gcc -c -I src/include -masm=intel -nostdlib -ffreestanding -mno-red-zone -fno-stack-protector src/common/util.c -o build/common/util.o
	x86_64-elf-gcc -c -I src/include -masm=intel -nostdlib -ffreestanding -mno-red-zone -fno-stack-protector -mgeneral-regs-only src/common/keyboard.c -o build/common/keyboard.o
	x86_64-elf-gcc -c -I src/include -masm=intel -nostdlib -ffreestanding -mno-red-zone -fno-stack-protector src/common/apic.c -o build/common/apic.o
	x86_64-elf-gcc -c -I src/include -masm=intel -nostdlib -ffreestanding -mno-red-zone -fno-stack-protector src/common/paging.c -o build/common/paging.o
	x86_64-elf-gcc -c -I src/include -masm=intel -nostdlib -ffreestanding -mno-red-zone -fno-stack-protector src/common/multiboot2.c -o build/common/multiboot2.o
	x86_64-elf-gcc -c -I src/include -masm=intel -nostdlib -ffreestanding -mno-red-zone -fno-stack-protector src/common/memory_physical.c -o build/common/memory_physical.o
	x86_64-elf-gcc -c -I src/include -masm=intel -nostdlib -ffreestanding -mno-red-zone -fno-stack-protector src/common/port.c -o build/common/port.o
	x86_64-elf-gcc -c -I src/include -masm=intel -nostdlib -ffreestanding -mno-red-zone -fno-stack-protector src/common/cpu.c -o build/common/cpu.o
	x86_64-elf-gcc -c -I src/include -masm=intel -nostdlib -ffreestanding -mno-red-zone -fno-stack-protector src/common/serial.c -o build/common/serial.o
	x86_64-elf-gcc -c -I src/include -masm=intel -nostdlib -ffreestanding -mno-red-zone -fno-stack-protector src/common/lock.c -o build/common/lock.o
	x86_64-elf-gcc -c -I src/include -masm=intel -nostdlib -ffreestanding -mno-red-zone -fno-stack-protector src/common/entrypoint.c -o build/common/entrypoint.o
	x86_64-elf-gcc -c -I src/include -masm=intel -nostdlib -ffreestanding -mno-red-zone -fno-stack-protector src/common/scheduler.c -o build/common/scheduler.o
# Compile using -mgeneral-regs-only so we can use gcc's interrupt attribute (see interrupt.c), this is needed because gcc only preserves general purpose registers and not
# SEE, MMX and x87 registers and states
	x86_64-elf-gcc -c -I src/include -masm=intel -nostdlib -ffreestanding -mno-red-zone -fno-stack-protector -mgeneral-regs-only src/common/idt.c -o build/common/idt.o
	x86_64-elf-gcc -c -I src/include -masm=intel -nostdlib -ffreestanding -mno-red-zone -fno-stack-protector -mgeneral-regs-only src/common/gdt.c -o build/common/gdt.o
	x86_64-elf-gcc -c -I src/include -masm=intel -nostdlib -ffreestanding -mno-red-zone -fno-stack-protector -mgeneral-regs-only src/common/main.c -o build/common/main.o
	ld -n -o build/x86_64/os.bin -T linker.ld build/x86_64/*.o build/common/*.o  

	mkdir build/x86_64/multiboot2 build/x86_64/multiboot2/boot build/x86_64/multiboot2/boot/grub
	cp grub.cfg build/x86_64/multiboot2/boot/grub
	cp build/x86_64/os.bin build/x86_64/multiboot2/boot/os.bin
	grub-mkrescue /usr/lib/grub/i386-pc -o build/x86_64/build.iso build/x86_64/multiboot2

run: build
	qemu-system-x86_64 -smp 4 -chardev stdio,id=char0,logfile=serial.log,signal=off -serial chardev:char0 -usb -m 1G build/x86_64/build.iso