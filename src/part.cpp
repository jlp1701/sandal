#include <def.h>
#include <part.h>
#include <sys.h>

bool isBootsectorValid(const Bootsector* bs) {
    // check boot signature
    if (bs->signature != 0xAA55) {
        return false;
    }
    return true;
}

const MbrPartition* getBootPartition(const Bootsector* bs) {
    // check all partitions if the boot flag is set
    for (unsigned long i = 0; i < 4; ++i)
    {
        const MbrPartition* p = &bs->partitions[i];
        if (p->status == 0x80) {
            return p;
        }
    }
    return NULL;
}

bool isExt2Formatted(const MbrPartition* p) {
    // read in first sector of superblock (512 bytes at offset 1024) and check bytes 56 - 57 for signature 0xef53
    unsigned long sbSector = p->firstLba + 2; 
    char buffer[512];
    if (readDisk(sbSector, 0, 512, &buffer) != 512) {
        return false;
    }
    // check signature
    if (*((unsigned short*)(&buffer[56])) != 0xef53) {
        return false;
    }
    return true;
}