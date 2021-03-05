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
        part.o(.bss)
        sys.o(.bss)
        fs.o(.bss)
    }
    PROVIDE(bss_end = .);
    . = 0x7c00;
    PROVIDE(text_start = .);
    .text :
    {
        b4.o(.text)
        k.o(.text)
        part.o(.text)
        sys.o(.text)
        fs.o(.text)
    }
    .rodata :
    {
        b4.o(.rodata)
        k.o(.rodata)
        part.o(.rodata)
        sys.o(.rodata)
        fs.o(.rodata)
    }
    .data :
    {
        b4.o(.data)
        k.o(.data)
        part.o(.data)
        sys.o(.data)
        fs.o(.data)
    }
    PROVIDE(data_end = .);
    . = 0x100000+0x7c00 - 1;
    .bla :
    {
        BYTE(0xDD)
    }
}
ASSERT(bss_end < 0x7c00, "Bss section overlaps with text section.");
ASSERT(data_end - text_start <= 11 * 512, "Stage 1 is too big. Load more sectors at stage 0.")