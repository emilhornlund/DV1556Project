#include "blockdevice.h"

BlockDevice::BlockDevice() {
    this->freePointer = 0;
}

BlockDevice::BlockDevice(const BlockDevice &other) {
    this->freePointer = other.freePointer;

    for (int i = 0; i < MEM_SIZE; ++i)
        this->memBlocks[i] = other.memBlocks[i];
}

BlockDevice::~BlockDevice() {

}

