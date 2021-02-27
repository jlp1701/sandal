ENTRY(boot);
OUTPUT_FORMAT("elf32-i386");
OUTPUT_ARCH("i386")
SECTIONS {
    . = 0x1000;
    .bss :
    {
        b4.o(.bss)
        k.o(.bss)
    }
    . = 0x7c00;
    .text :
    {
        b4.o(.text)
        k.o(.text)
    }
    .rodata :
    {
        b4.o(.rodata)
        k.o(.rodata)
    }
    .data :
    {
        b4.o(.data)
        k.o(.data)
    }
    . = 0x100000+0x7c00 - 1;
    .bla :
    {
        BYTE(0xDD)
    }
}