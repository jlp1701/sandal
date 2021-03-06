#ifndef SYS_H
#define SYS_H

#define VGA_ROWS    25
#define VGA_COLS    80
#define DISK_READ   0x42
#define DISK_WRITE  0x43
#define BYTES_PER_SEC   512

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

void lineToBuffer(VgaBuffer* vgaBuf, const char* strLine);
void initBuffer(VgaBuffer* vgaBuf, const char pad);
void printBuffer(VgaBuffer* vgaBuf);
void printStr(VgaBuffer* vgaBuf, const char* str);
void memcpy(const void* dest, const void* src, const unsigned long num);
long strncmp(const char* str1, const char* str2, unsigned long num);
unsigned long readDiskSector(const unsigned long lba);
unsigned long readDiskSector(const unsigned long lba, const unsigned long offset, const unsigned long size, const void** buf);
unsigned long readDisk(const unsigned long lba, const unsigned long offset, const unsigned long size, const void* buf);

#endif