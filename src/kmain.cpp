#include <def.h>
#include <kmain.h>
#include <part.h>
#include <fs.h>
#include <sys.h>


extern "C" void kmain()
{
    static VgaBuffer vgaBuffer;
    const unsigned long lba_kernel = (unsigned long)LBA_KERNEL;
    unsigned long numSec = 0;
    void* const baseAddr = (void*)RM_KERNEL_BASE_PTR;
    void* const protKernelAddr = (void*)PM_KERNEL_ADDR;

    initBuffer(&vgaBuffer, ' ');
    printBuffer(&vgaBuffer);

    // identify postion and size of linux kernel
    // for this example we assume its at 0x10000 (= lba 2048) on the disk with size of 8785656 bytes (= 17160 sectors)

    // check bootsector of disk and get partitions
    // map bootsector struct to load address of bootsector
    
    const Bootsector* const bs = (Bootsector*)0x7c00;

    if (!isBootsectorValid(bs)) {
        printStr(&vgaBuffer, "Invalid bootsector.");
        return;
    }
    // get the parition with "boot" or "active" flag
    const MbrPartition* const bootPart = getBootPartition(bs);
    if (bootPart == NULL) {
        printStr(&vgaBuffer, "No partition marked as boot partition.");
        return;
    }
    
    // check for known file system
    if (!isExt2Formatted(bootPart)) {
        printStr(&vgaBuffer, "Boot partition format is unknown (known are: ext2).");
        return;
    }

    // check and try to read file of linux kernel
    const char* kernelFilepath[] = {"boot", "vmlinuz-5.8.0-44-generic", NULL};
    Ext2Fs ext2fs(bootPart);
    if (!ext2fs.fileExists(kernelFilepath)) {
        printStr(&vgaBuffer, "Unable to find linux kernel on file system.");
        return;
    }

    auto kernelFileSize = ext2fs.getFileSize(kernelFilepath);
    if (kernelFileSize == 0) {
        printStr(&vgaBuffer, "Invalid kernel file size.");
        return;   
    }
    
    // load kernel file from lba 2048 at 0x100000 (1 MB)
    if (readDisk(lba_kernel, 0, kernelFileSize , (void*)0x100000) != kernelFileSize) {
        printStr(&vgaBuffer, "Error while trying to load linux kernel file (fixed sector).");
        return;
    }

    // load kernell file from fs to 0x1000000 (16 MB)
    if (ext2fs.readFile(kernelFilepath, 0, kernelFileSize, (void*)0x1000000) != kernelFileSize) {
        printStr(&vgaBuffer, "Error while trying to load linux kernel file (file system).");
        return;
    }

    // compare both files
    for (unsigned long i = 0; i < kernelFileSize; i++) {
        if (*(((char*)0x100000) + i) != *(((char*)0x1000000) + i)) {
            printStr(&vgaBuffer, "Files differ.");
            return;
        }
    }

    // load linux kernel boot sector (first 512 bytes of kernel image)
    printStr(&vgaBuffer, "Load linux boot sector ...");
    if (ext2fs.readFile(kernelFilepath, 0, 512, baseAddr) != 512) {
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
    if (ext2fs.readFile(kernelFilepath, 512, sects * BYTES_PER_SEC, (char*)baseAddr + 512) != sects * BYTES_PER_SEC) {
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
    if (ext2fs.readFile(kernelFilepath, (sects + 1) * BYTES_PER_SEC, numSec * BYTES_PER_SEC, protKernelAddr) != numSec * BYTES_PER_SEC) {
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



