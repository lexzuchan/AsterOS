CC := gcc
LD := ld
CFLAGS := -m32 -std=gnu11 -ffreestanding -O2 -Wall -Wextra -fno-stack-protector -fno-pie -fno-pic -fno-builtin -fno-asynchronous-unwind-tables
LDFLAGS := -m elf_i386 -T linker.ld -nostdlib

.PHONY: all iso check run clean
all: build/kernel.elf

build:
	mkdir -p build

build/boot.o: src/boot.S | build
	$(CC) -m32 -c -o $@ $<

build/kernel.o: src/kernel.c | build
	$(CC) $(CFLAGS) -c -o $@ $<

build/kernel.elf: build/boot.o build/kernel.o linker.ld
	$(LD) $(LDFLAGS) -o $@ build/boot.o build/kernel.o

check: build/kernel.elf
	grub-file --is-x86-multiboot build/kernel.elf
	@echo 'Multiboot v1 check passed.'

iso: check
	mkdir -p build/isofiles/boot/grub
	cp build/kernel.elf build/isofiles/boot/kernel.elf
	cp iso/boot/grub/grub.cfg build/isofiles/boot/grub/grub.cfg
	grub-mkrescue -o build/asteros.iso build/isofiles
	@test -s build/asteros.iso
	@echo 'ISO ready: build/asteros.iso'

run: iso
	qemu-system-i386 -m 128M -cdrom build/asteros.iso -boot d

clean:
	rm -rf build
