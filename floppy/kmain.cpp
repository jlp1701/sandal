#define VGA_ROWS    25
#define VGA_COLS    80
#define PAD_CHAR    0x30    // padding char for text console; 0x30 = space
#define DISK_READ   0x42
#define DISK_WRITE  0x43
#define NUM_CYLINDERS   1024
#define NUM_HEADS       255
#define NUM_SECTORS     63
#define BYTES_PER_SEC   512
#define HEAP_SIZE       0x6000
struct __attribute__((__packed__)) VgaBuffer {
    unsigned short buf[VGA_ROWS][VGA_COLS];
    unsigned short curRow;
    char padChar;
};

struct __attribute__((__packed__)) DapStruct {
    unsigned char size;
    unsigned char pad;
    unsigned short numSectors;
    unsigned short offset;
    unsigned short segment;
    unsigned long lba_lower;
    unsigned long lba_higher;
};

struct __attribute__((__packed__)) LinuxBootHeaderStruct {
    unsigned char setup_sects;
    unsigned short root_flags;
    unsigned long syssize;
    unsigned short ram_size;
    unsigned short vid_mode;
    unsigned short root_dev;
    unsigned short boot_flag;
};

extern "C" char Disk_IO(unsigned long action,
                        DapStruct* dap);

extern "C" void callKernel(unsigned short real_mode_code_seg,
                            unsigned short kernel_entry_seg,
                            unsigned long real_mode_and_heap_size);

extern "C" unsigned long tmp_buffer_ptr;

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

void memcpy(void* dest, void* src, unsigned long num) {
    for (unsigned long i = 0; i < num; i++) {
        *((char*)dest + i) = *((char*)src + i);
    }
}

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

char readDiskSectors(unsigned long lbaBegin, unsigned long numSec, void* destAddr) {    
    char err = 0;
    DapStruct dap;
    for (unsigned long i = 0; i < numSec; i++) {
        dap.size = 16;
        dap.pad = 0;
        dap.numSectors = 1;
        dap.offset = (unsigned short)((unsigned long)&tmp_buffer_ptr);
        dap.segment = 0;
        dap.lba_lower = lbaBegin + i;
        dap.lba_higher = 0;
        err = Disk_IO(DISK_READ, &dap);
        if (err) {
            return err;
        }
        memcpy((char*)destAddr + i * BYTES_PER_SEC, (void*)&tmp_buffer_ptr, BYTES_PER_SEC);
    }
    return 0;
}

extern "C" void kmain()
{
    static VgaBuffer vgaBuffer;
    const short color = 0x0F00;
    const char* hello = "Hello cpp world!";
    unsigned short* vga = (unsigned short*)0xb8000;
    unsigned long lba_prog = 2048;
    // unsigned long lba_prog = 1986;
    unsigned long numSec = 19016;
    void* lbhAddr = (void*)0x9000;
    void* protKernelAddr = (void*)0x100000;

    // unsigned long size = 4096;

    for (int i = 0; i<16;++i)
        vga[i+80] = color | hello[i];


    initBuffer(&vgaBuffer, ' ');
    printBuffer(&vgaBuffer);

    // identify postion and size of linux kernel
    // for this example we assume its at 0xf8400 (= lba 1986) with size of 8785656 bytes (= 17160 sectors)


    // load linux kernel boot sector (first 512 bytes of kernel image)
    printStr(&vgaBuffer, "Load linux boot sector ...");
    if (readDiskSectors(lba_prog, 1, lbhAddr) != 0) {
        printStr(&vgaBuffer, "Error while trying to load linux boot sector.");
        return;
    }
    // check "MZ" signature
    if (*((unsigned short*)lbhAddr) != 0x5A4D) {
        printStr(&vgaBuffer, "Invalid signature of prog sectors.");
    }
    printStr(&vgaBuffer, "Loaded linux boot sector.");
    
    // map LinuxBootHeader structure onto loaded sector and check signature
    LinuxBootHeaderStruct* lbh = (LinuxBootHeaderStruct*)((unsigned char*)lbhAddr + 0x1F1);
    if (lbh->boot_flag != 0xAA55) {
        printStr(&vgaBuffer, "Linux boot flag invalid.");
        return;
    }

    // load kernel real-mode code (setup code)
    unsigned char sects = lbh->setup_sects;
    if (sects == 0) {
        sects = 4;
    }
    printStr(&vgaBuffer, "Load linux real-mode code ...");
    if (readDiskSectors(lba_prog, sects + 1/*40*/, (char*)lbhAddr) != 0) {
        printStr(&vgaBuffer, "Error while trying to load linux real-mode code.");
        return;
    }
    printStr(&vgaBuffer, "Loaded linux real-mode code.");

    // set neccessary parameters of kernel boot sector
    // set type of loader to 0xFF (= undefined)
    *(((unsigned char*)lbhAddr) + 0x210) = 0xFF;

    // get loadflags
    unsigned char loadflags;
    loadflags = *(((unsigned char*)lbhAddr) + 0x211);
    if (!(loadflags & (1 << 0))) {
        printStr(&vgaBuffer, "Linux kernels that load at 0x10000 are not supported");
        return;
    }
    loadflags &= ~(1 << 5);     // print early messages
    loadflags |=  (1 << 7);     // can use heap
    *(((unsigned char*)lbhAddr) + 0x211) = loadflags;

    // set heap_end_ptr (0xFE00)
    unsigned long real_mode_and_heap_size = 0x8000 + HEAP_SIZE;
    *(unsigned short*)(((unsigned char*)lbhAddr) + 0x224) = real_mode_and_heap_size - 0x200;

    // set command line to "auto" and set pointer
    *(((char*)lbhAddr) + 0x10000 + 0) = 'a';
    *(((char*)lbhAddr) + 0x10000 + 1) = 'u';
    *(((char*)lbhAddr) + 0x10000 + 2) = 't';
    *(((char*)lbhAddr) + 0x10000 + 3) = 'o';
    *(((char*)lbhAddr) + 0x10000 + 4) = 0;
    *(unsigned long*)(((unsigned char*)lbhAddr) + 0x228) = (unsigned long)lbhAddr + 0x10000;    

    // load protected-mode kernel
    printStr(&vgaBuffer, "Load linux 32bit kernel ...");
    if (readDiskSectors(lba_prog + sects, numSec - sects, protKernelAddr) != 0) {
        printStr(&vgaBuffer, "Error while trying to load linux 32bit kernel.");
        return;
    }
    printStr(&vgaBuffer, "Loaded linux 32bit kernel.");
    
    // setup correct register values for kernel

    // jump to kernel entry point
    unsigned short real_mode_code_seg = (0x9000 >> 4);
    unsigned short kernel_entry_seg   = real_mode_code_seg + 0x20;
    printStr(&vgaBuffer, "Calling linux kernel ...");
    callKernel(real_mode_code_seg, kernel_entry_seg, real_mode_and_heap_size);
    //((void (*)())((unsigned long)lbhAddr + 0x20))();


    // printStr(&vgaBuffer, "Load prog ...");
    // if (readDiskSectors(lba_prog, numSec, lbhAddr) != 0) {
    //     printStr(&vgaBuffer, "Error while trying to load prog.");
    //     return;
    // }
    // if (*((unsigned char*)lbhAddr + numSec * BYTES_PER_SEC - 1) != 0xDD) {
    //     printStr(&vgaBuffer, "Invalid signature of prog sectors.");
    // }
    // printStr(&vgaBuffer, "Prog loaded");
    // ((void (*)())0x100000)();

    // // check if sectors read are equal
    // char* a = (char*)0xE000;
    // char* b = (char*)0x00100000;
    // for (int i = 0; i < 512; i++) {
    //     if (*(a+i) != *(b+i)) {
    //         printStr(&vgaBuffer, "Memory content is not equal!");
    //         break;
    //     }
    // }
    // printStr(&vgaBuffer, "Disk read check finished.");
}





