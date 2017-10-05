#include "filesystem.h"
#include "INode.h"

#include <iostream>
#include "Split.h"
#include <vector>

//#define DEBUG_FILESYSTEM

FileSystem::FileSystem() {
    this->currentINode = NULL;
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

    this->currentINode = this->inodes[this->createInode(0, "/", 7, "root", "root", "/", true, false)];
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

int FileSystem::findInodeByName(int* inodeIndexes, std::string* filenames, int size, std::string searchFilename) {
    bool found = false;
    int i = 0;

    for (; i < size && !found; i++) {
        if(filenames[i] == searchFilename) {
            found = true;
        }
    }

    if (found) return inodeIndexes[i-1];
    else return -1;
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

/*int FileSystem::createFolder(std::string filepath, std::string username) {
    int retVal = -1;

    bool found = false;
    int i = (int)(filepath.length() - 1);
    for (; i > 0 && !found; i--) {
        if (filepath[i] == '/') {
            found = true;
        }
    }

    std::string filename = filepath.substr((i+2), filepath.length());
    filepath.erase(i+1, filepath.length() - 1);

    if (!filename.empty()) {
        INode *directory = NULL;
        if (found) {
            int inodeIndex = goToFolder(filepath);
            if (inodeIndex < 0) {
                throw std::out_of_range("No such filepath");
            }
            directory = this->inodes[inodeIndex];

            if (!directory->isDir()) throw "create: cannot create: Not a directory";

        } else {
            directory = this->currentINode;
        }
        retVal = this->createInode(directory->getThisInodeIndex(), filename, 7, username, username, directory->getPWD() + filename + "/", true, false);
    }
    return retVal;
}*/

int FileSystem::createFile(std::string filepath, std::string username, const bool isDir) {
    int retVal = -1;

    bool found = false;
    int i = (int)(filepath.length() - 1);
    for (; i > 0 && !found; i--) {
        if (filepath[i] == '/') {
            found = true;
        }
    }

    std::string filename;
    int n = i;
    if(found) {
        n = i+2;
    }
    filename = filepath.substr((n), filepath.length());

    if (!filename.empty()) {
        INode *directory = NULL;
        if (found) {
            filepath.erase(i+1, filepath.length());
            std::cout << "FilePath: " << filepath << std::endl;
            int inodeIndex = goToFolder(filepath);
            if (inodeIndex < 0) {
                throw std::out_of_range("No such filepath");
            }
            directory = this->inodes[inodeIndex];

            if (!directory->isDir()) throw "create: cannot create: Not a directory";

        } else {
            directory = this->currentINode;
        }

        std::string newPWD = directory->getPWD() + filename;
        if (isDir) {
            newPWD += "/";
        }

        retVal = this->createInode(directory->getThisInodeIndex(), filename, 7, username, username, newPWD, isDir, false);
    }
    return retVal;
}

int FileSystem::createInode(unsigned short int parentInodeIndex, std::string filename, unsigned short int protection, std::string creator, std::string owner, std::string pwd, bool isDir, bool isHidden) {
    int inodeIndex = this->findNextFreeInode();



    if (this->currentINode != NULL) {
        if(filename.length() < 14) {
            INode *parent = this->inodes[parentInodeIndex];

            int tmpFileSize = parent->getFilesize();
            if (tmpFileSize + 16 < 512) {
                std::string tmpDataBlock = this->mMemblockDevice.readBlock(parent->getFirstDataBlockIndex()).toString();

                char data[16];
                data[0] = (char)inodeIndex;

                int i = 0;
                for (; i < filename.length(); i++) {
                    data[(i + 1)] = filename[i];
                }

                if(i < 15) {
                    data[++i] = '\0';
                }

                char* tmpData = new char[tmpDataBlock.size()];
                std::copy(tmpDataBlock.begin(), tmpDataBlock.end(), tmpData);

                this->appendData(tmpData, tmpFileSize, data, 16);

                //for(int i = 0; i < 512; i++)
                    //std::cout << tmpData[i] << std::endl;

                this->writeData(parent->getFirstDataBlockIndex(), tmpData);
                parent->setFilesize(parent->getFilesize()+16);
                delete[] tmpData;
            }
        } else {
            //abort
            inodeIndex = -1;
        }
    }

    int dataIndex = this->findNextFreeData();
    int tmpFileSize = 0;

    if (inodeIndex != -1 && dataIndex != -1) {
        this->bitmapINodes[inodeIndex] = true;
        this->bitmapData[dataIndex] = true;

        this->inodes[inodeIndex] = new INode(parentInodeIndex, inodeIndex, filename, protection, creator, owner, pwd, tmpFileSize, isDir);
        this->inodes[inodeIndex]->setFilesize(0);
        this->inodes[inodeIndex]->setDataBlock(dataIndex);

        if (isDir) {

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
    } else {
        inodeIndex = -1;
    }
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

int FileSystem::writeData(int dataBlock, char *data) {
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
    if(blockIndex < 0 || blockIndex >= 512) {
        throw std::out_of_range("Exception: out of range");
    }
    return this->mMemblockDevice.readBlock(blockIndex).toString();
}

int FileSystem::goToFolder(std::string filePath) {
    int inodeIndex = -1;
    INode* startFromInode = this->currentINode;                 // Relative path

#ifdef DEBUG_FILESYSTEM
    std::cout << "FilePath: " << filePath << std::endl;
#endif

    if (filePath[0] == '/') {                                   // Absolute path - Start from root (0)
        startFromInode = this->inodes[0];
        filePath.erase(0, 1);
    }

    std::vector<std::string> names;

    std::string tmpStr;
    for (int i = 0; i < filePath.length(); i++) {
        if (filePath[i] != '/') {
            tmpStr += filePath[i];
        } else {
            names.push_back(tmpStr);
            tmpStr = "";
        }
    }

    names.push_back(tmpStr);

#ifdef DEBUG_FILESYSTEM
    for(int i = 0; i < names.size(); i++) {
        std::cout << "Name: " << names[i] << std::endl;
    }
#endif

    int i = 0;
    for (; i < names.size(); i++) {
        int* inodeIndexes;
        std::string* directories;
        int nrOfDirectories = this->getAllDirectoriesFromDataBlock(startFromInode, inodeIndexes, directories);

        inodeIndex = findInodeByName(inodeIndexes, directories, nrOfDirectories, names[i]);

        delete[] inodeIndexes;
        inodeIndexes = NULL;
        delete[] directories;
        directories = NULL;

        if (inodeIndex != -1 && this->inodes[inodeIndex]->isDir()) {
            startFromInode = this->inodes[inodeIndex];
            if (!startFromInode->isDir()) {
                throw "Not a directory";
            }
        } else {
            inodeIndex = -1;
            break;
        }
    }

    return inodeIndex;
}

int FileSystem::moveToFolder(const std::string filepath) {
    int inodeIndex = this->goToFolder(filepath);
    if (inodeIndex != -1) {
        this->currentINode = this->inodes[inodeIndex];
    }
    return inodeIndex;
}

std::string FileSystem::cat(const std::string filepath) {
    std::string content;
    char* actualData;
    int inodeIndex = this->goToFolder(filepath);
    if (inodeIndex != -1) {
        INode *inode = this->inodes[inodeIndex];
        int fileSize = inode->getFilesize();

        int blockIndex = inode->getFirstDataBlockIndex();
        content += this->openData(blockIndex);

        blockIndex = inode->getNextDataBlockIndex();
        while (blockIndex != -1) {
            content += this->openData(blockIndex);
            blockIndex = inode->getNextDataBlockIndex();
        }

        actualData = new char[fileSize];
        for (int i = 0; i < fileSize; i++) {
            actualData[i] = content[i];
        }
    }
    return actualData;
}

