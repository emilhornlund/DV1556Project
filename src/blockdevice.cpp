#include "blockdevice.h"

BlockDevice::BlockDevice(int nrOfBlocks) {
    this->freePointer = 0;
}

BlockDevice::BlockDevice(const BlockDevice &other) {
    this->freePointer = other.freePointer;

    for (int i = 0; i < 250; ++i)
        this->memBlocks[i] = other.memBlocks[i];
}

BlockDevice::~BlockDevice() {

}

