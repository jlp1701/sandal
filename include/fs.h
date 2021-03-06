#ifndef FS_H
#define FS_H

#include <part.h>

#define MAX_FILE_FRAGMENTS  5


struct __attribute__((__packed__)) FileFragment
{
    unsigned long begin;
    unsigned long end;
};

struct __attribute__((__packed__)) INode {
    unsigned short typeAndPerm;
    unsigned short UserId;
    unsigned long sizeLower;
    unsigned long lastAccessTime;
    unsigned long creationTime;
    unsigned long lastModTime;
    unsigned long delTime;
    unsigned short GroupId;
    unsigned short numHardLinks;
    unsigned long numDiskSectors;
    unsigned long flags;
    unsigned long osSpec1;
    unsigned long directBlocks[12];
    unsigned long singleIndirectBlock;
    unsigned long doubleIndirectBlock;
    unsigned long tripleIndirectBlock;
    unsigned long genNum;
    unsigned long extAttributeBlock;
    unsigned long sizeUpper;
    unsigned long fragmentBlock;
    unsigned char osSpec2[12];
};

struct __attribute__((__packed__)) Ext2SuperBlock {
    unsigned long numTotalInodes;
    unsigned long numTotalBlocks;
    unsigned long numReservedBlocks;
    unsigned long numUnallocBlocks;
    unsigned long numUnallocInodes;
    unsigned long firstBlockOfGroup0;
    unsigned long blockSize;
    unsigned long fragmentSize;
    unsigned long blocksPerGroup;
    unsigned long fragmentsPerGroup;
    unsigned long inodesPerGroup;
    unsigned long lastMountTime;
    unsigned long lastWriteTime;
    unsigned short mountsSinceLastCheck;
    unsigned short mountsToNextCheck;
    unsigned short signature;
    unsigned short state;
    unsigned short errorAction;
    unsigned short versionMinor;
    unsigned long lastCheckTime;
    unsigned long checkInterval;
    unsigned long osId;
    unsigned long versionMajor;
    unsigned short resBlockUserId;
    unsigned short resBlockGroupId;
    // extended fields
    unsigned long firstNonReservedINode;
    unsigned short sizeOfINodeStruct;
    unsigned short blockGroupOfThisSB;
    char _res[164]; // align size to 512 block size
};

struct __attribute__((__packed__)) GroupDesc {
    unsigned long blockBitmapBlock;
    unsigned long iNodeBitmapBlock;
    unsigned long iNodeBlockStart;
    unsigned short numUnallocBlocks;
    unsigned short numUnallocINodes;
    unsigned short numDirs;
    char _res[14];
};

#define MAX_NAME_LEN    256
#define DE_TYPE_REGULAR     1
#define DE_TYPE_DIRECTORY     2
struct __attribute__((__packed__)) DirEntry {
    unsigned long iNode;
    unsigned short entrySize;
    unsigned char nameLen;
    unsigned char type;
    char name[MAX_NAME_LEN];
};

struct __attribute__((__packed__)) DirEntryStub {
    unsigned long iNode;
    unsigned short entrySize;
    unsigned char nameLen;
    unsigned char type;
};

struct __attribute__((__packed__)) Ext2ReadStruct {
    unsigned long offset;
    unsigned long tOff;
    unsigned long totalSize;
    unsigned long readSize;
    void* destBuf;
};

class File
{
public:
    File();
    ~File();
    unsigned long read(unsigned long offset, unsigned long size, void* buffer);
    FileFragment fragments[MAX_FILE_FRAGMENTS];
    unsigned long size;
private:
};

class Ext2Fs
{
public:
    Ext2Fs(const MbrPartition*);
    ~Ext2Fs();

    unsigned long readFile(const char** filePath, unsigned long offset, unsigned long size, void* buf);
    bool fileExists(const char** filePath);
    //unsigned long readFile(unsigned long offset, unsigned long size, void* buffer);
private:
    MbrPartition part;
    Ext2SuperBlock sb;

    unsigned long getNumberOfGroups();
    unsigned long getBlockSize();
    unsigned long getNumINodesPerBlock();
    unsigned long getLbaOfFirstBlock();
    unsigned long getBlockGroupOfINode(unsigned long inode);
    bool readINodeEntry(unsigned long inode, INode* inodePtr);
    unsigned long blockNo2Lba(unsigned long blockNo);
    unsigned long path2INode(unsigned long currNode, const char** filePath);
    unsigned long searchDir(INode* dirInodeData, const char* name);
    unsigned long setFileData(unsigned long inode, File* fp);
    unsigned long readBlockData(unsigned long blockIdx, unsigned long offset, unsigned long size, void* buf);
    bool readGroupDesc(unsigned long groupNum, GroupDesc* gd);
    bool searchBlockForDirEntry(unsigned long blockIdx, unsigned long indirectionLvl, unsigned long& remSize, const char* name, DirEntry* dp);
    bool visitBlock(unsigned long blockIdx, unsigned long indirectionLvl, unsigned long& blockOffset, Ext2ReadStruct* rd);
    bool readINodeData(INode* iNodeStruct, Ext2ReadStruct* rd);
};


#endif