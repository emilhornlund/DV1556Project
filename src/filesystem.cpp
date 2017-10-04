#include "filesystem.h"
#include "INode.h"

#include <iostream>

FileSystem::FileSystem() {
	this->init();
}

FileSystem::~FileSystem() {
	this->reset();
}

void FileSystem::init () {
    for (int i = 0; i < MAX_INT; i++) {
        this->inodes[i] = NULL;
    }

    this->bitmapDataIndex = 0;
    this->bitmapInodeIndex = 0;

    this->currentINode = this->inodes[this->createInode(0, "/", 7, "root", "root", "/", 0, true)];
}

void FileSystem::format () {
    this->reset();
    this->init();
}

int FileSystem::findNextFreeData() {
    short int retVal = -1;
    int stopIndex = (this->bitmapDataIndex - 1) % MAX_INT;
    bool found = false;

    for (; this->bitmapDataIndex != stopIndex && !found; this->bitmapDataIndex++) {
        if (this->bitmapDataIndex >= MAX_INT) this->bitmapDataIndex = 0;

        if (!this->bitmapData[this->bitmapDataIndex]) {
            retVal = this->bitmapDataIndex;
            found = true;
        }
    }

    return retVal;
}

int FileSystem::findNextFreeInode() {
    short int retVal = -1;
    int stopIndex = (this->bitmapInodeIndex - 1) % MAX_INT;
    bool found = false;

    for (; this->bitmapInodeIndex != stopIndex && !found; this->bitmapInodeIndex++) {
        if (this->bitmapInodeIndex >= MAX_INT) this->bitmapInodeIndex = 0;

        if (!this->bitmapINodes[this->bitmapInodeIndex]) {
            retVal = this->bitmapInodeIndex;
            found = true;
        }
    }

    return retVal;
}

int FileSystem::findInodeByName(std::string filename) {
    int i = 0;
    bool found = false;

    for(; i < MAX_INT && !found; i++)
        if(this->inodes[i]->getFilename() == filename) found = true;

    if (!found) i = -1;

    return i;
}

void FileSystem::resetINodes() {
	for (int i = 0; i < MAX_INT; i++) {
		if (this->inodes[i] != NULL) {
			delete this->inodes[i];
			this->inodes[i] = NULL;
		}
	}
	this->currentINode = NULL;
}

void FileSystem::resetBitmapINodes() {
	for (int i = 0; i < MAX_INT; i++) {
		this->bitmapINodes[i] = false;
	}
}

void FileSystem::resetBitmapData() {
	for (int i = 0; i < MAX_INT; i++) {
		this->bitmapData[i] = false;
	}
}

void FileSystem::reset() {
	this->resetINodes();
	this->resetBitmapINodes();
	this->resetBitmapData();
}

std::string FileSystem::getPWD() const {
	return this->currentINode->getPWD();
}

int FileSystem::createInode(unsigned short int parentInodeIndex, std::string filename, unsigned short int protection, std::string creator, std::string owner, std::string pwd, unsigned short int filesize, bool isDir, bool isHidden) {
    int inodeIndex = this->findNextFreeInode();
    int dataIndex = this->findNextFreeData();
    int tmpFileSize = 0;

    if (inodeIndex != -1 && dataIndex != -1) {
        this->bitmapINodes[inodeIndex] = true;
        this->bitmapData[dataIndex] = true;

        this->inodes[inodeIndex] = new INode(parentInodeIndex, filename, protection, creator, owner, pwd, filesize, isDir);
        this->inodes[inodeIndex]->setFilesize(0);
        this->inodes[inodeIndex]->setDataBlock(dataIndex);

        if(isDir) {

            char data[16] = {inodeIndex, '.', '\0'};
            char data2[16] = {parentInodeIndex, '.', '.', '\0'};
            char dataBlock[512];

            appendData(dataBlock, tmpFileSize, data, 16);
            tmpFileSize += 16;

            appendData(dataBlock, tmpFileSize, data2, 16);
            tmpFileSize += 16;

            this->inodes[inodeIndex]->setFilesize(tmpFileSize);
            this->inodes[inodeIndex]->setDataBlock(dataIndex);
            this->writeData(dataIndex, dataBlock);

            //for(int i = 0; i < 512; i++)
                //std::cout << dataBlock[i] << std::endl;

        }
    } else
        inodeIndex = -1;

    return inodeIndex;
}

int FileSystem::_appendData (char* dataBlock, int currentBlockSize, const char* data, int dataSize) {
    int iLow = 0;

    for (int i = currentBlockSize; i < (currentBlockSize + dataSize); i++) {
        dataBlock[i] = data[iLow];
        iLow++;
    }

    return dataSize;
}

int FileSystem::appendData (char* dataBlock, int currentBlockSize, const char* data, int dataSize) {
    int sizeRemainingInBlock = 512 - currentBlockSize;
    int retVal = -1;

    if (sizeRemainingInBlock >= dataSize) {
        retVal = this->_appendData(dataBlock, currentBlockSize, data, dataSize);
    } else {
        if(sizeRemainingInBlock > 0) {
            retVal = this->_appendData(dataBlock, currentBlockSize, data, sizeRemainingInBlock);
        }
    }

    return retVal;
}

int FileSystem::writeData(int dataBlock, char* data) {
    this->mMemblockDevice.writeBlock(dataBlock, data);
}

int FileSystem::getAllDirectoriesFromDataBlock (INode* inode, int* &inodes, std::string* &directories) {
    int nrOfDirs = inode->getFilesize()/16;
    int dataBlockIndex = inode->getFirstDataBlockIndex();
    directories = NULL;
    inodes      = NULL;

    if (dataBlockIndex != -1) {
        directories = new std::string[nrOfDirs];
        inodes      = new int[nrOfDirs];

        std::string dataBlock = this->openData(dataBlockIndex);

        for (int i = 0; i < nrOfDirs; i++) {
            std::string dir = "";
            for (int n = (i * 16); n < ((i * 16) + 16); n++) {
                if(n == (i * 16)) {
                    inodes[i] = (int)dataBlock[n];
                } else {
                    if(dataBlock[n] != '\0')
                        dir += dataBlock[n];
                    else
                        break;
                }
            }
            directories[i] = dir;
        }
    }

    return nrOfDirs;
}

int FileSystem::listDir (int* &inodes, std::string* &directories) {
    return this->getAllDirectoriesFromDataBlock(this->currentINode, inodes, directories);
}

std::string FileSystem::openData(int blockIndex) {
    if(blockIndex < 0 && blockIndex >= 512) {
        throw std::out_of_range("Exception: out of range");
    }

    return this->mMemblockDevice.readBlock(blockIndex).toString();
}