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
    unsigned long lbaFsStart = this->part.firstLba;
    readDisk(lbaFsStart + 2, 0, sizeof(Ext2SuperBlock), &this->sb);  // superblock is alwas at offset 1024 with length of 1024
}

Ext2Fs::~Ext2Fs() {};


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
    unsigned long firstINodeBlock = gd.iNodeBlockStart;

    // block offset in INode blocks
    unsigned long blockOffset = ((inode - 1) % this->sb.inodesPerGroup) * this->sb.sizeOfINodeStruct;

    // read the data into the INode structure
    if (readBlockData(firstINodeBlock, blockOffset, sizeof(INode), inodePtr) != sizeof(INode)) {
        return false;
    }
    // TODO: how to check if read data is fishy?

    return true;
}

bool Ext2Fs::searchBlockForDirEntry(unsigned long blockIdx, unsigned long indirectionLvl, unsigned long& remSize, const char* name, DirEntry* dp) {
    DirEntryStub ds;
    if (indirectionLvl == 0) {  // direct
        unsigned long n = 0;
        while (n < getBlockSize()) {
            // get dir entry stub
            readBlockData(blockIdx, n, sizeof(DirEntryStub), &ds);
            if (ds.iNode != 0 && (ds.type == DE_TYPE_REGULAR || ds.type == DE_TYPE_DIRECTORY)) {
                // read name
                if (ds.nameLen > MAX_NAME_LEN - 1) {
                    // we have a problem due to static size of names
                    remSize = 0;
                    return false;
                }
                // read dir entry with name again
                readBlockData(blockIdx, n, sizeof(DirEntryStub) + ds.nameLen + 1, dp);
                if (strncmp(name, dp->name, ds.nameLen) == 0) {
                    // entry match!
                    return true;
                }
            }
            n += ds.entrySize;
            if (ds.entrySize > remSize) {
                remSize = 0;
            } else {
                remSize -= ds.entrySize;
            }
            if (remSize == 0) {
                // end of size to read and file was not found --> error
                return false;
            }
        }
    } else if (indirectionLvl > 0) {
        unsigned long dirBlockIdx = 0;
        for (unsigned long i = 0; i < getBlockSize(); i += 4) {
            if (remSize > 0) {
                readBlockData(blockIdx, i, 4, &dirBlockIdx);
                if (dirBlockIdx == 0) {
                    return false;
                }
                bool ret = searchBlockForDirEntry(dirBlockIdx, indirectionLvl - 1, remSize, name, dp);
                if (ret == true) {
                    return true;
                }
            }
        }
    }
    return false;
}

unsigned long Ext2Fs::searchDir(INode* dirINodeData, const char* name) {
    unsigned long remSize = dirINodeData->sizeLower;  // TODO: handle sizeUpper
    DirEntry de;
    // search direct blocks
    for (unsigned long i = 0; i < 12; i++) {
        auto ret = searchBlockForDirEntry(dirINodeData->directBlocks[i], 0, remSize, name, &de);
        if (ret) {
            return de.iNode;
        }
        if (remSize == 0) {
            return 0;
        }
    }

    // search single indirect block
    if (remSize > 0) {
        auto found = searchBlockForDirEntry(dirINodeData->singleIndirectBlock, 1, remSize, name, &de);
        if (found) {
            return de.iNode;
        }
    }

    // search double indirect block
    if (remSize > 0) {
        auto found = searchBlockForDirEntry(dirINodeData->doubleIndirectBlock, 2, remSize, name, &de);
        if (found) {
            return de.iNode;
        }
    }

    // seach triple indirect block
    if (remSize > 0) {
        auto found = searchBlockForDirEntry(dirINodeData->tripleIndirectBlock, 3, remSize, name, &de);
        if (found) {
            return de.iNode;
        }
    }
    return 0;
}

bool Ext2Fs::visitBlock(unsigned long blockIdx, unsigned long indirectionLvl, unsigned long& blockOffset, Ext2ReadStruct* rd) {
    if (indirectionLvl == 0) {
        if (rd->tOff < this->getBlockSize()) {
            unsigned long toRead = rd->totalSize - rd->readSize;
            if (toRead + rd->tOff > this->getBlockSize()) {
                toRead = this->getBlockSize() - rd->tOff;
            }
            unsigned long numRead = readBlockData(blockIdx, rd->tOff, toRead, ((char*)rd->destBuf) + rd->readSize);
            rd->tOff = 0;
            rd->readSize += numRead;
            if (rd->readSize >= rd->totalSize) {
                blockOffset += this->getBlockSize();
                return true;
            }
            blockOffset += this->getBlockSize();
            return false;

        } else {
            rd->tOff -= this->getBlockSize();
            blockOffset += this->getBlockSize();
            return false;
        }

    } else {
        unsigned long indirBlockIdx = 0;
        for (unsigned long i = 0; i < this->getBlockSize(); i += 4) {
            readBlockData(blockIdx, i, 4, &indirBlockIdx);
            if (indirBlockIdx != 0) {
                auto fin = visitBlock(indirBlockIdx, indirectionLvl - 1, blockOffset, rd);
                if (fin) {
                    blockOffset += this->getBlockSize();
                    return true;
                }
            }
        }
        blockOffset += this->getBlockSize();
        return false;
    }
}

bool Ext2Fs::readINodeData(INode* iNodeStruct, Ext2ReadStruct* rd) {
    unsigned long blockOffset = 0;
    // visit direct blocks
    for (unsigned long i = 0; i < 12; i++) {
        auto fin = visitBlock(iNodeStruct->directBlocks[i], 0, blockOffset, rd);
        if (fin) {
            return true;
        }
    }

    // search single indirect block
    auto fin = visitBlock(iNodeStruct->singleIndirectBlock, 1, blockOffset, rd);
    if (fin) {
        return true;
    }

    // search double indirect block
    fin = visitBlock(iNodeStruct->doubleIndirectBlock, 2, blockOffset, rd);
    if (fin) {
        return true;
    }

    // seach triple indirect block
    fin = visitBlock(iNodeStruct->tripleIndirectBlock, 3, blockOffset, rd);
    if (fin) {
        return true;
    }
    return false;
}

unsigned long Ext2Fs::setFileData(unsigned long inode, File* fp) {
    fp = fp;
    inode = inode;
    return 0;
}

unsigned long Ext2Fs::readFile(const char** filePath, unsigned long offset, unsigned long size, void* buf) {
    // get iNode of path
    unsigned long iNode = path2INode(2, filePath);
    if (iNode == 0) {
        return 0;
    }
    INode nodeEntry;
    readINodeEntry(iNode, &nodeEntry);
    Ext2ReadStruct rd;
    rd.offset = offset;
    rd.tOff = offset;
    rd.totalSize = size;
    rd.readSize = 0;
    rd.destBuf = buf;
    auto success = readINodeData(&nodeEntry, &rd);
    if (success) {
        return rd.readSize;
    }
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
    unsigned long fsSecOffset = (blockIdx + i) * (this->getBlockSize() / BYTES_PER_SEC);
    return readDisk(this->part.firstLba + fsSecOffset, off, size, buf);
}

unsigned long Ext2Fs::getBlockSize() {
    return (1024 << this->sb.blockSize);
}