#define VGA_ROWS    25
#define VGA_COLS    80
#define PAD_CHAR    0x30    // padding char for text console; 0x30 = space
#define DISK_READ   0x02
#define DISK_WRITE  0x03
#define NUM_CYLINDERS   1024
#define NUM_HEADS       255
#define NUM_SECTORS     63
extern "C" char Disk_IO(unsigned long action,
                        unsigned long numSectors,
                        unsigned long chs,
                        unsigned int addr);

struct __attribute__((__packed__)) VgaBuffer {
    unsigned short buf[VGA_ROWS][VGA_COLS];
    unsigned short curRow;
    char padChar;
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
    if (*p != 0 && i >= VGA_COLS) {
       lineToBuffer(vgaBuf, p);    // print remainder of string to new line
    } else {
        // fill up remaining line with pad char
        while (i < VGA_COLS) {
            vgaBuf->buf[vgaBuf->curRow][i] = color | (unsigned short)vgaBuf->padChar;
            i++;
        }
    }
}

void initBuffer(VgaBuffer* vgaBuf, char pad) {
    const unsigned short color = 0xF00;  // predefined color
    vgaBuf->padChar = pad;
    for (unsigned short r = 0; r < VGA_ROWS; r++) {
        for (unsigned short c = 0; c < VGA_COLS; c++) {
            vgaBuf->buf[r][c] = color | (unsigned short)pad;
        }
    }
}

void delay(unsigned long c) {
    volatile unsigned long t = 0;
    for (unsigned long i = 0; i < c; i++) {
        t++;
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
            //delay(1000000);
        }
        r_off++;
    }
}

void printStr(VgaBuffer* vgaBuf, const char* str) {
    lineToBuffer(vgaBuf, str);
    printBuffer(vgaBuf);
}

// struct __attribute__((__packed__)) ChsStruct {
//     short
// };

// translates linear block address (LBA) to cylinder, head, sector (CHS)
unsigned long lba2chs(unsigned long lba) {
    unsigned short c = lba / NUM_HEADS / NUM_SECTORS;
    unsigned short h = lba / NUM_SECTORS - c * NUM_HEADS;
    unsigned short s = lba - (NUM_SECTORS * (c * NUM_HEADS + h)) + 1;

    // c, h, s are packed into one dword
    unsigned short cs = ((c & 0xFF) << 8) | ((c & 0x0300) >> 2) | ((s & 0x003F));  // cylinder and sector -> CX
    unsigned long chs = (cs << 16) | (h & 0x00FF); // head --> DH
    return chs;
}

extern "C" void kmain()
{
    static VgaBuffer vgaBuffer;
    const short color = 0x0F00;
    const char* hello = "Hello cpp world!";
    unsigned short* vga = (unsigned short*)0xb8000;
    // // char bios_ret = 0;

    // delay(100000000);
    for (int i = 0; i<16;++i)
        vga[i+80] = color | hello[i];


    initBuffer(&vgaBuffer, ' ');
    printBuffer(&vgaBuffer);

    // lineToBuffer(&vgaBuffer, bla);
    // lineToBuffer(&vgaBuffer, hello);
    // lineToBuffer(&vgaBuffer, "adadasasdasdasdasdadasadadasasdasdasdasdadadasasdasdasdasdadasadadasasdasdasdasturadadasasdasdasdasdadasadadasasdasdasdasdadadasasdasdasdasdadasadadasasdasdasdastur");
    // lineToBuffer(&vgaBuffer, "abc");
    // char c = '0';
    // for (int i = 0; i < 25; i++) {
    //     c = 'a' + i;
    //     printStr(&vgaBuffer, (const char*)&c);
    // }

    // printStr(&vgaBuffer, "Begin disk read check ...");
    // for (int i = 0; i < 256; i++) {
    //     printStr(&vgaBuffer, "Error when trying to read from disk.");    
    // }
    // printStr(&vgaBuffer, "Disk read check finished.");

    printStr(&vgaBuffer, "Begin disk read check ...");
    if (Disk_IO(DISK_READ, 1, lba2chs(48321), 0xE000) != 0) {
        printStr(&vgaBuffer, "Error when trying to read from disk.");
        return;
    }
    if (*(unsigned short*)(0xE000+510) != 0xAA55) {
        printStr(&vgaBuffer, "Invalid MBR signature.");
        // return;
    }

    if (Disk_IO(DISK_READ, 1, lba2chs(48321), 0xF000) != 0) {
        printStr(&vgaBuffer, "Error when trying to read from disk.");
        return;
    }
    // check if sectors read are equal
    char* a = (char*)0xE000;
    char* b = (char*)0xF000;
    for (int i = 0; i < 256; i++) {
        if (*(a+i) != *(b+i)) {
            printStr(&vgaBuffer, "Memory content is not equal!");
            break;
        }
    }
    printStr(&vgaBuffer, "Disk read check finished.");
}





