BINDIR = ../bin
LIBDIR = ../lib
INCDIR = ../include
SRCDIR = .

CFLAGS = -O0 -g3 -F dwarf -c
CFLAGS_SPEC = -m32 -fno-pie -ffreestanding -std=c++11 -mno-red-zone -fno-exceptions -nostdlib -fno-rtti -Wall -Wextra -Werror

.PHONY: kernel append run install runimg debug clean

kernel: $(BINDIR)/kernel.elf $(BINDIR)/kernel.bin

$(BINDIR)/kernel.elf: $(INCDIR)/* $(SRCDIR)/*
	nasm -f elf32 -g3 -F dwarf boot.asm -o $(BINDIR)/b4.o
	g++ kmain.cpp  $(CFLAGS) -o $(BINDIR)/k.o -I $(INCDIR) $(CFLAGS_SPEC)
	g++ part.cpp $(CFLAGS) -o $(BINDIR)/part.o -I $(INCDIR) $(CFLAGS_SPEC)
	g++ sys.cpp $(CFLAGS) -o $(BINDIR)/sys.o -I $(INCDIR) $(CFLAGS_SPEC)
	g++ fs.cpp $(CFLAGS) -o $(BINDIR)/fs.o -I $(INCDIR) $(CFLAGS_SPEC)
# 	nasm -f elf32 boot.asm -o $(BINDIR)/b4.o
# 	g++ kmain.cpp -O0 -c -o $(BINDIR)/k.o -I $(INCDIR) -m32 -fno-pie -ffreestanding -std=c++11 -mno-red-zone -fno-exceptions -nostdlib -fno-rtti -Wall -Wextra -Werror
	ld -T linker_kernel.ld -L $(BINDIR) -o $(BINDIR)/kernel.elf

$(BINDIR)/kernel.bin: $(BINDIR)/kernel.elf
	cd $(BINDIR) && objcopy -O binary  kernel.elf kernel.bin

append: clean kernel $(LIBDIR)/vmlinuz-5.0.0-36-generic
	cat $(LIBDIR)/vmlinuz-5.0.0-36-generic >> $(BINDIR)/kernel.bin

run: kernel
	qemu-system-i386 -drive format=raw,file=$(BINDIR)/kernel.bin &

install: kernel
	cp -u $(LIBDIR)/ubuntu.img $(BINDIR)/ubuntu.img
	cp -u $(LIBDIR)/vmlinuz-5.8.0-44-generic $(BINDIR)/vmlinuz-5.8.0-44-generic
	cd $(BINDIR)/ && dd conv=notrunc if=kernel.bin of=ubuntu.img bs=1 count=446
	cd $(BINDIR)/ && dd conv=notrunc if=kernel.bin skip=1 of=ubuntu.img seek=1 bs=512 count=13
	cd $(BINDIR)/ && dd conv=notrunc if=vmlinuz-5.8.0-44-generic of=ubuntu.img seek=2048 bs=512

runimg: install
	qemu-system-x86_64 -m 1024 -machine type=pc,accel=kvm -drive format=raw,file=$(BINDIR)/ubuntu.img &


debug: kernel
	qemu-system-i386 -drive format=raw,file=$(BINDIR)/ubuntu.img -S -s &
	gdb $(BINDIR)/kernel.elf \
	        -ex 'target remote localhost:1234' \
	        -ex 'set architecture i8086' \
	        -ex 'set disassembly-flavor intel' \
	        -ex 'layout src' \
	        -ex 'layout regs' \
	        -ex 'break boot' \
	        -ex 'continue'

clean: 
	cd $(BINDIR) && rm -f kernel.elf kernel.bin b4.o k.o




