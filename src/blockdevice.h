#ifndef BLOCKDEVICE_H
#define BLOCKDEVICE_H

/*
 * Pure virtual class
 */

#include "block.h"

class BlockDevice
{
public:
    const static int MEM_SIZE = 250;
protected:
    Block memBlocks[MEM_SIZE];
    int freePointer;
public:
    BlockDevice();
    BlockDevice(const BlockDevice &other);

    virtual ~BlockDevice();
    virtual int spaceLeft() const = 0;
    virtual int writeBlock(int blockNr, const std::vector<char> &vec) = 0;
    virtual int writeBlock(int blockNr, const std::string &strBlock) = 0;
    virtual int writeBlock(int blockNr, const char cArr[]) = 0;
    virtual int writeBlock(int blockNr, const Block block) = 0;
    virtual Block readBlock(int blockNr) const = 0;
    virtual void reset() = 0;
    virtual int size() const = 0;
};

#endif // BLOCKDEVICE_H
