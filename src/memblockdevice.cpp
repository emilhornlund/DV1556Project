#include "memblockdevice.h"
#include <stdexcept>

MemBlockDevice::MemBlockDevice(): BlockDevice() {

}

MemBlockDevice::MemBlockDevice(const MemBlockDevice &other) : BlockDevice(other) {

}

MemBlockDevice::~MemBlockDevice() {
    /* Implicit call to base-class destructor */
}

MemBlockDevice& MemBlockDevice::operator=(const MemBlockDevice &other) {
    this->freePointer = other.freePointer;

    for (int i = 0; i < MEM_SIZE; ++i)
        this->memBlocks[i] = other.memBlocks[i];

    return *this;
}

const Block& MemBlockDevice::operator[](int index) const {
    if (index < 0 || index >= MEM_SIZE) {
        throw std::out_of_range("Illegal access\n");
    }
    else {
        return (this->memBlocks[index]);
    }
}

int MemBlockDevice::spaceLeft() const {
    /* Not yet implemented */
    return 0;
}

int MemBlockDevice::writeBlock(int blockNr, const std::vector<char> &vec) {
    int output = -1;    // Assume blockNr out-of-range

    if (blockNr < MEM_SIZE && blockNr >= 0) {
        /* -2 = vec and block dont have same dimensions */
        /* 1 = success */
        output = this->memBlocks[blockNr].writeBlock(vec);
    }
    return output;
}

int MemBlockDevice::writeBlock(int blockNr, const std::string &strBlock) {
    int output = -1;    // Assume blockNr out-of-range

    if (blockNr < 250 && blockNr >= 0) {
        /* -2 = str-length and block dont have same dimensions */
        /* 1 = success */
        output = this->memBlocks[blockNr].writeBlock(strBlock);
    }
    return output;
}

int MemBlockDevice::writeBlock(int blockNr, const char cArr[]) {
    int output = -1;    // Assume blockNr out-of-range
    if (blockNr < 250 && blockNr >= 0) {
        output = 1;
        // Underlying function writeBlock cannot check array-dimension.
        this->memBlocks[blockNr].writeBlock(cArr);
    }
    return output;
}

int MemBlockDevice::writeBlock(int blockNr, const Block block) {
    int retVal = -1;
    if(blockNr < MEM_SIZE && blockNr >= 0) {
        this->memBlocks[blockNr] = block;
        retVal = 0;
    }

    return retVal;
}

Block MemBlockDevice::readBlock(int blockNr) const {
    if (blockNr < 0 || blockNr >= MEM_SIZE)
        throw std::out_of_range("Block out of range");
    else {
        Block a(this->memBlocks[blockNr]);
        return a;
    }
}

/* Resets all the blocks */
void MemBlockDevice::reset() {
    for (int i = 0; i < MEM_SIZE; ++i) {
        this->memBlocks[i].reset('0');
    }
}

int MemBlockDevice::size() const {
    return MEM_SIZE;
}
