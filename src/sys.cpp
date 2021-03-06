#include <def.h>
#include <sys.h>

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

void initBuffer(VgaBuffer* vgaBuf, const char pad) {
    const unsigned short color = 0xF00;  // predefined color
    vgaBuf->padChar = pad;
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

void memcpy(const void* dest, const void* src, const unsigned long num) {
    for (unsigned long i = 0; i < num; i++) {
        *((char*)dest + i) = *((char*)src + i);
    }
}

long strncmp(const char* str1, const char* str2, unsigned long num) {
    long d = 0;
    for (unsigned long i = 0; i < num; i++) {
        d = *(str1 + i) - *(str2 + i);
        if (d != 0 || (*(str1 + i) == 0 && *(str2 + i) == 0)) {
            return d;
        }
    }
    return d;
}

unsigned long readDiskSector(const unsigned long lba, const unsigned long offset, const unsigned long size, const void** buf) {    
    unsigned long i = offset / BYTES_PER_SEC;
    unsigned long off = offset - i * BYTES_PER_SEC;
    char err = 0;
    DapStruct dap;
    dap.size = 16;
    dap.pad = 0;
    dap.numSectors = 1;
    dap.offset = (unsigned short)((unsigned long)&tmp_buffer_ptr);
    dap.segment = 0;
    dap.lba_lower = lba + i;
    dap.lba_higher = 0;
    err = Disk_IO(DISK_READ, &dap);
    if (err) {
        return 0;
    }
    *buf = (char*)(&tmp_buffer_ptr) + off;
    unsigned long numRead = BYTES_PER_SEC - off;
    if (numRead > size) {
        numRead = size;
    }
    return numRead;
}

unsigned long readDiskSector(const unsigned long lba, const void** buf) {
    return readDiskSector(lba, 0, BYTES_PER_SEC, buf);
}

unsigned long readDisk(const unsigned long lba, const unsigned long offset, const unsigned long size, const void* buf) {
    unsigned long n = 0;
    unsigned long i = 0;
    unsigned long toRead = size;
    // check if offset is larger than sector size
    i = offset / BYTES_PER_SEC;
    unsigned long off = offset - i * BYTES_PER_SEC;
    const void* tBuf = NULL;
    while (n < size) {
        unsigned long delta = readDiskSector(lba + i, off, toRead, &tBuf);
        if (delta == 0) {
            return 0;
        }
        memcpy(((char*)buf) + n, (char*)tBuf, delta);
        off = 0;
        n += delta;
        toRead -= delta;
        i++;
    }
    return n;
}