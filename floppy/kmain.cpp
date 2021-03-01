#define VGA_ROWS    25
#define VGA_COLS    80
#define DISK_READ   0x42
#define DISK_WRITE  0x43
#define BYTES_PER_SEC   512

// user defined variables
#define LBA_KERNEL          2048
#define RM_KERNEL_BASE_PTR  0x9000
#define HEAP_SIZE           0x6000
#define PM_KERNEL_ADDR      0x100000
#define KERNEL_CMD_LINE     "root=/dev/sda2 S"

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

extern "C" __attribute__((__cdecl__)) char Disk_IO(const unsigned long action,
                        const DapStruct* dap);

extern "C" __attribute__((__cdecl__)) void callKernel(const unsigned short real_mode_code_seg,
                           const unsigned short kernel_entry_seg,
                           const unsigned long real_mode_and_heap_size);

extern "C" const unsigned long tmp_buffer_ptr;

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

char readDiskSectors(const unsigned long lbaBegin, const unsigned long numSec, const void* destAddr) {    
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
    const unsigned long lba_kernel = (unsigned long)LBA_KERNEL;
    unsigned long numSec = 0;
    const void* baseAddr = (void*)RM_KERNEL_BASE_PTR;
    const void* protKernelAddr = (void*)PM_KERNEL_ADDR;

    initBuffer(&vgaBuffer, ' ');
    printBuffer(&vgaBuffer);

    // identify postion and size of linux kernel
    // for this example we assume its at 0x10000 (= lba 2048) on the disk with size of 8785656 bytes (= 17160 sectors)

    // load linux kernel boot sector (first 512 bytes of kernel image)
    printStr(&vgaBuffer, "Load linux boot sector ...");
    if (readDiskSectors(lba_kernel, 1, baseAddr) != 0) {
        printStr(&vgaBuffer, "Error while trying to load linux boot sector.");
        return;
    }
    // check "MZ" signature
    if (*((unsigned short*)baseAddr) != 0x5A4D) {
        printStr(&vgaBuffer, "Invalid signature of prog sectors.");
    }
    printStr(&vgaBuffer, "Loaded linux boot sector.");
    
    // map LinuxBootHeader structure onto loaded sector and check signature
    LinuxBootHeaderStruct* lbh = (LinuxBootHeaderStruct*)((unsigned char*)baseAddr + 0x1F1);
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
    if (readDiskSectors(lba_kernel + 1, sects, (char*)baseAddr + 512) != 0) {
        printStr(&vgaBuffer, "Error while trying to load linux real-mode code.");
        return;
    }
    printStr(&vgaBuffer, "Loaded linux real-mode code.");

    // set neccessary parameters of kernel boot sector
    // set type of loader to 0xFF (= undefined)
    *(((unsigned char*)baseAddr) + 0x210) = 0xFF;  // loader type
    // *(((unsigned char*)baseAddr) + 0x227) = 0x11;  // extended loader type

    // get loadflags
    unsigned char loadflags;
    loadflags = *(((unsigned char*)baseAddr) + 0x211);
    if (!(loadflags & (1 << 0))) {
        printStr(&vgaBuffer, "Linux kernels that load at 0x10000 are not supported");
        return;
    }
    loadflags &= ~(1 << 5);     // print early messages
    loadflags |=  (1 << 7);     // can use heap
    *(((unsigned char*)baseAddr) + 0x211) = loadflags;

    // set heap_end_ptr (0xFE00)
    const unsigned long real_mode_and_heap_size = 0x8000 + HEAP_SIZE;
    *(unsigned short*)(((unsigned char*)baseAddr) + 0x224) = real_mode_and_heap_size - 0x200;

    // set command line to "auto" and set pointer
    const char* cmd_line = KERNEL_CMD_LINE;
    const char* cmd_line_ptr = (char*)baseAddr + real_mode_and_heap_size;
    memcpy(cmd_line_ptr, (void*)cmd_line, 17);
    *(unsigned long*)(((unsigned char*)baseAddr) + 0x228) = (unsigned long)cmd_line_ptr;

    // get size of protected mode code
    numSec = *(unsigned long*)(((unsigned char*)baseAddr) + 0x1F4);
    numSec <<= 4;  // now the size is in bytes
    numSec /= 512;  // now its in sectors
    numSec += 1;

    // load protected-mode kernel
    printStr(&vgaBuffer, "Load linux 32bit kernel ...");
    if (readDiskSectors(lba_kernel + sects + 1, numSec, protKernelAddr) != 0) {
        printStr(&vgaBuffer, "Error while trying to load linux 32bit kernel.");
        return;
    }
    printStr(&vgaBuffer, "Loaded linux 32bit kernel.");

    // jump to kernel entry point
    const unsigned short real_mode_code_seg = ((unsigned long)baseAddr >> 4);
    const unsigned short kernel_entry_seg   = real_mode_code_seg + 0x20;
    printStr(&vgaBuffer, "Calling linux kernel ...");
    callKernel(real_mode_code_seg, kernel_entry_seg, real_mode_and_heap_size);
}





