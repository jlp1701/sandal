ENTRY(boot);
OUTPUT_FORMAT("elf32-i386");
OUTPUT_ARCH("i386")
SECTIONS {
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
    . = 0xA000;
    .bss :
    {
        b4.o(.bss)
        k.o(.bss)
    }
    . = 0x00100000;
    .prog : 
    {
        prog.o(.text)
        prog.o(.rodata)
        prog.o(.data)
        prog.o(.bss)
    }
    . = 0x00100200-1;
    .bla : 
    {
        BYTE(0xAA)
    }
}
