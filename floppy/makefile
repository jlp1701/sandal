

kernel: kernel.elf kernel.bin

kernel.elf: boot4.asm kmain.cpp linker_kernel.ld
	nasm -f elf32 -g3 -F dwarf boot4.asm -o b4.o
	g++ kmain.cpp -O0 -g3 -F dwarf -c -o k.o -m32 -fno-pie -ffreestanding -std=c++11 -mno-red-zone -fno-exceptions -nostdlib -fno-rtti -Wall -Wextra -Werror
	ld -T linker_kernel.ld b4.o k.o -o kernel.elf

kernel.bin: kernel.elf
	objcopy -O binary kernel.elf kernel.bin

run: kernel
	qemu-system-i386 -hda kernel.bin &

debug: kernel
	qemu-system-i386 -hda kernel.bin -S -s &
	gdb kernel.elf \
	        -ex 'target remote localhost:1234' \
	        -ex 'set architecture i8086' \
	        -ex 'set disassembly-flavor intel' \
	        -ex 'layout src' \
	        -ex 'layout regs' \
	        -ex 'break boot' \
	        -ex 'continue'

clean: 
	rm kernel.elf kernel.bin b4.o k.o




