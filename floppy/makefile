

kernel: kernel.elf kernel.bin

kernel.elf: boot4.asm kmain.cpp linker_kernel.ld
# 	nasm -f elf32 -g3 -F dwarf boot4.asm -o b4.o
# 	g++ kmain.cpp -Os -g3 -F dwarf -c -o k.o -m32 -fno-pie -ffreestanding -std=c++11 -mno-red-zone -fno-exceptions -nostdlib -fno-rtti -Wall -Wextra -Werror
	nasm -f elf32 boot4.asm -o b4.o
	g++ kmain.cpp -Os -c -o k.o -m32 -fno-pie -ffreestanding -std=c++11 -mno-red-zone -fno-exceptions -nostdlib -fno-rtti -Wall -Wextra -Werror
	ld -T linker_kernel.ld b4.o k.o -o kernel.elf

kernel.bin: kernel.elf
	objcopy -O binary kernel.elf kernel.bin

append: clean kernel vmlinuz-5.0.0-36-generic
	cat vmlinuz-5.0.0-36-generic >> kernel.bin

run: kernel
	qemu-system-i386 -hda kernel.bin &

install: kernel ubuntu.img
	dd conv=notrunc if=kernel.bin of=ubuntu.img bs=1 count=446
	dd conv=notrunc if=kernel.bin skip=1 of=ubuntu.img seek=1 bs=512 count=7
	dd conv=notrunc if=vmlinuz-5.8.0-44-generic of=ubuntu.img seek=2048 bs=512

runimg: install
	qemu-system-x86_64 -m 1024 -machine type=pc,accel=kvm -drive format=raw,file=ubuntu.img &


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
	rm -f kernel.elf kernel.bin b4.o k.o




