#ifndef PART_H
#define PART_H

#define FLAG_BOOT   (1 << 7)

struct __attribute__((__packed__)) ChsAddress {
    unsigned char cylinder;
    unsigned char head;
    unsigned char sector;
};

struct __attribute__((__packed__)) MbrPartition {
    unsigned char status;
    ChsAddress firstSector;
    unsigned char type;
    ChsAddress lastSector;
    unsigned long firstLba;
    unsigned long numSectors;
};

struct __attribute__((__packed__)) Bootsector {
    unsigned char bootcode[446];
    MbrPartition partitions[4];
    unsigned short signature;
};

bool isBootsectorValid(const Bootsector* bs);
const MbrPartition* getBootPartition(const Bootsector* bs);
bool isExt2Formatted(const MbrPartition*);

#endif