#define VGA_ROWS    25
#define VGA_COLS    80
#define PAD_CHAR    0x30    // padding char for text console; 0x30 = space
extern "C" void ReadSectorFromDisk(unsigned long numToRead, unsigned long head, unsigned long sector, unsigned long cylinder, unsigned int targetAddr);

struct VgaBuffer {
    unsigned short buf[VGA_ROWS][VGA_COLS];
    unsigned short curRow;
};

void lineToBuffer(VgaBuffer* vgaBuf, const char* strLine) {
    const unsigned short color = 0xF00;  // predefined color
    char* p = (char*)strLine;
    unsigned short i = 0;
    // set new line pos and copy line to buffer
    vgaBuf->curRow = (vgaBuf->curRow + 1) % VGA_ROWS;
    while (*p != 0 && i < VGA_COLS) {
        vgaBuf->buf[vgaBuf->curRow][i] = color | (unsigned short)*p;
        p++;
        i++;
    }

    // check if line is longer than one row
    if (*p != 0 && i >= VGA_COLS)
       lineToBuffer(vgaBuf, p);    // print remainder of string to new line
}

void initBuffer(VgaBuffer* vgaBuf, char pad) {
    const unsigned short color = 0xF00;  // predefined color
    for (unsigned short r = 0; r < VGA_ROWS; r++) {
        for (unsigned short c = 0; c < VGA_COLS; c++) {
            vgaBuf->buf[r][c] = color | (unsigned short)pad;
        }
    }
}

void printBuffer(VgaBuffer* vgaBuf) {
    unsigned short* vga = (unsigned short*)0xb8000;
    unsigned short r_off = vgaBuf->curRow + 1;
    for (unsigned short r = 0; r < VGA_ROWS; r++) {
        for (unsigned short c = 0; c < VGA_COLS; c++) {
            if (r_off >= VGA_ROWS)
                r_off = 0;
            vga[r * VGA_COLS + c] = (unsigned short)vgaBuf->buf[r_off][c];
        }
        r_off++;
    }
}

void printStr(VgaBuffer* vgaBuf, const char* str) {
    lineToBuffer(vgaBuf, str);
    printBuffer(vgaBuf);
}

extern "C" void kmain()
{
    static VgaBuffer vgaBuffer;
    const short color = 0x0F00;
    const char* hello = "Hello cpp world!";
    //const char bla[] = {'L', 'O', 'L', 0x00};
    unsigned short* vga = (unsigned short*)0xb8000;
    for (int i = 0; i<16;++i)
        vga[i+80] = color | hello[i];

    ReadSectorFromDisk(1, 0, 1, 0, 0xE000);

    initBuffer(&vgaBuffer, ' ');
    printBuffer(&vgaBuffer);

    // lineToBuffer(&vgaBuffer, bla);
    // lineToBuffer(&vgaBuffer, hello);
    // lineToBuffer(&vgaBuffer, "adadasasdasdasdasdadasadadasasdasdasdasdadadasasdasdasdasdadasadadasasdasdasdasturadadasasdasdasdasdadasadadasasdasdasdasdadadasasdasdasdasdadasadadasasdasdasdastur");
    // lineToBuffer(&vgaBuffer, "abc");
    char c = '0';
    for (int i = 0; i < 25; i++) {
        c = 'a' + i;
        printStr(&vgaBuffer, (const char*)&c);
    }
}





