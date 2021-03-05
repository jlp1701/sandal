#include <def.h>
#include <fs.h>
#include <sys.h>

File::File() {};
File::~File() {};
unsigned long File::read(unsigned long offset, unsigned long size, void* buffer) {
    return offset + size + (unsigned long)buffer;
}


Ext2Fs::Ext2Fs(const MbrPartition* p) {
    this->part = *p;
    // read superblock
}

Ext2Fs::~Ext2Fs() {};

bool Ext2Fs::getFileHandle(const char** filePath, File* fileHandle) {
    unsigned long nodeNum = path2INode(2, filePath);  // start at root dir
    if (nodeNum == 0) {
        return false;
    }
    unsigned long frags = setFileData(nodeNum, fileHandle);
    if (frags == 0) {
        return false;
    }
    return true;
}

bool Ext2Fs::fileExists(const char** filePath) {
    unsigned long nodeNum = path2INode(2, filePath);  // start at root dir 
    if (nodeNum != 0) {
        return true;
    }
    return false;
}

unsigned long Ext2Fs::path2INode(unsigned long currNode, const char** filePath) {
    // filePath must at least have two entries: ["name", 0]

    // read inode data
    INode nodeEntry;
    readINodeEntry(currNode, &nodeEntry);

    // check directory entries for folder or file
    unsigned long nodeNum = searchDir(&nodeEntry, *filePath);
    if (nodeNum == 0) {
        return false;
    }
    // if found: get inode and repeat until file is found
    // check if it was the last path part
    if (*(filePath + 1) == NULL) {
        return nodeNum;
    } else {
        return path2INode(nodeNum, filePath + 1);
    }
}

unsigned long Ext2Fs::getBlockGroupOfINode(unsigned long inode) {
    return inode / this->sb.inodesPerGroup;
}

bool Ext2Fs::readINodeEntry(unsigned long inode, INode* inodePtr) {
    // locate block group of inode
    inodePtr = inodePtr;
    unsigned long groupNum = getBlockGroupOfINode(inode);

    // read group descriptor of corresponding block
    GroupDesc gd;
    readGroupDesc(groupNum, &gd);

    // get first INode block

    // get index of INode block

    // get index of INode entry

    // read the data into the INode structure

    return true;
}

unsigned long Ext2Fs::searchDir(INode* dirINodeData, const char* name) {
    dirINodeData = dirINodeData;
    name = name;

    return 0;
}

unsigned long Ext2Fs::setFileData(unsigned long inode, File* fp) {
    fp = fp;
    inode = inode;
    return 0;
}

bool Ext2Fs::readGroupDesc(unsigned long groupNum, GroupDesc* gd) {
    // locate group descriptor block
    unsigned long gdBlockStart;
    if (this->getBlockSize() == 1024) {
        gdBlockStart = 2;
    } else {
        gdBlockStart = 1;
    }

    // read the structure from disk
    unsigned long rd = readBlockData(gdBlockStart, groupNum * 32, 32, gd);
    if (rd == 0) {
        return false;
    } else {
        return true;
    }
}

unsigned long Ext2Fs::readBlockData(unsigned long blockIdx, unsigned long offset, unsigned long size, void* buf) {
    // convert blockIdx to offset in disk sectors
    unsigned long i = offset / this->getBlockSize();
    unsigned long off = offset - i * this->getBlockSize();
    unsigned long fsSecOffset = (blockIdx + i) * this->getBlockSize() / BYTES_PER_SEC;
    return readDisk(this->part.firstLba + fsSecOffset, off, size, buf);
}

unsigned long Ext2Fs::getBlockSize() {
    return (1024 << this->sb.blockSize);
}