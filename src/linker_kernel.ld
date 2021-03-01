ENTRY(boot);
OUTPUT_FORMAT("elf32-i386");
OUTPUT_ARCH("i386")
SECTIONS {
    . = 0x1000;
    PROVIDE(bss_start = .);
    .bss :
    {
        b4.o(.bss)
        k.o(.bss)
    }
    PROVIDE(bss_end = .);
    . = 0x7c00;
    PROVIDE(text_start = .);
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
    PROVIDE(data_end = .);
    . = 0x100000+0x7c00 - 1;
    .bla :
    {
        BYTE(0xDD)
    }
}
ASSERT(bss_end < 0x7c00, "Bss section overlaps with text section.");
ASSERT(data_end - text_start <= 8 * 512, "Stage 1 is too big. Load more sectors at stage 0.")