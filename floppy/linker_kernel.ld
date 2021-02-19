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
        *(.rodata)
    }
    .data :
    {
        *(.data)
    }
    .bss :
    {
        *(.bss)
    }
}