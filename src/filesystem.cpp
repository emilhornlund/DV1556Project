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

int FileSystem::findInodeIndexByName(std::string* filenames, int size, std::string searchFilename) {
    bool found = false;
    int i = 0;

    for (; i < size && !found; i++) {
        if(filenames[i] == searchFilename) {
            found = true;
        }
    }

    if (found) return i-1;
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

int FileSystem::fileExists(INode *inode, std::string filename) {
    int *inodeIndexes;
    std::string *directories;
    int nrOfDirs = this->getAllDirectoriesFromDataBlock(inode, inodeIndexes, directories);
    int inodeIndex = this->findInodeIndexByName(directories, nrOfDirs, filename);

    delete[] inodeIndexes;
    inodeIndexes = NULL;
    delete[] directories;
    directories = NULL;

    return inodeIndex;
}

int FileSystem::createFile(std::string filepath, std::string username, std::string &content, const bool isDir) {
    int retVal = -1;

    std::string filename;
    bool found = this->splitFilepath(filename, filepath);

    INode *location;
    if(found || filepath[0] == '/') {
        int inodeIndex = goToFolder(filepath);
        //std::cout << inodeIndex << std::endl;
        location = this->inodes[inodeIndex];
    } else {
        location = this->currentINode;
    }

    if (!location->isDir()) throw "create: cannot create: Not a directory";

    int inodeIndex = this->fileExists(location, filename);

    if (inodeIndex != -1) {
        throw "Error: folder / file exists.";
    }

    if (!filename.empty()) {
        std::string newPWD = location->getPWD() + filename;
        if (isDir) {
            newPWD += "/";
        }
        retVal = this->createInode(location->getThisInodeIndex(), filename, 7, username, username, newPWD, isDir, false);

        if (!isDir) {
            //SKAPA DATA
            //content.append('\0');
            int filesize = content.length();
            //std::cout << "FileSize: " << filesize << std::endl;

            int nrOfBlocks = filesize / 512;
            if (nrOfBlocks*512 < filesize || nrOfBlocks == 0)
                nrOfBlocks++;

            if (nrOfBlocks > 10) {
                throw "Error: data to large";
            }

            int* freeData = new int[nrOfBlocks];

            bool exist = true;
            for(int i = 0; i < nrOfBlocks; i++) {
                freeData[i] = this->findNextFreeData();
                if(freeData[i] == -1)
                    exist = false;
            }

            if(!exist) {
                delete this->inodes[retVal];
                this->inodes[retVal] = NULL;
                this->bitmapINodes[retVal] = false;
                throw "Error: no free spaces on harddrive.";
            }

            for(int i = 0; i < nrOfBlocks; i++) {
                this->bitmapData[freeData[i]] = true;
            }

            this->inodes[retVal]->setFilesize(filesize);

            for (int i = 0; i < nrOfBlocks; i++) {
                this->inodes[retVal]->setSpecificDataBlock(i, freeData[i]);
            }

            if (filesize <= 512) {
                std::string dataStr;
                dataStr = this->openData(this->inodes[retVal]->getFirstDataBlockIndex());

                char dataBlock[512] = {'\0'};
                char data[512] = {'\0'};

                for (int i = 0; i < filesize; i++) {
                    data[i] = content[i];
                }

                //std::cout << "Filesize: " << filesize << std::endl;
                this->appendData(dataBlock, 0, data, filesize);
                this->writeData(this->inodes[retVal]->getFirstDataBlockIndex(), dataBlock);
            } else {
                ///PART 1

                int blockIndex = this->inodes[retVal]->getFirstDataBlockIndex();

                std::string dataStr;
                dataStr = this->openData(blockIndex);

                char dataBlock[512] = {'\0'};
                char data[512];

                for (int i = 0; i < 512; i++) {
                    data[i] = content[i];
                    //std::cout << data[i];
                }

                this->appendData(dataBlock, 0, data, 512);
                this->writeData(blockIndex, dataBlock);
                filesize -= 512;

                /// PART 2

                //std::cout << std::endl << "Part2: FirstDataBlock" << std::endl;

                int counter = 1;
                while (filesize > 512) {
                    blockIndex = this->inodes[retVal]->getNextDataBlockIndex();
                    dataStr = this->openData(blockIndex);

                    char dataBlock[512] = {'\0'};
                    char data[512];

                    for (int i = 0; i < 512; i++) {
                        data[i] = content[(counter * 512) + i];
                        //std::cout << data[i];
                    }

                    this->appendData(dataBlock, 0, data, 512);
                    this->writeData(blockIndex, dataBlock);

                    //std::cout << std::endl << "Part2: FileSize Before: " << filesize << std::endl;
                    filesize -= 512;
                    //std::cout << "Part2: FileSize After: " << filesize << std::endl;

                    counter++;

                    //blockIndex = this->inodes[retVal]->getNextDataBlockIndex();
                }

                /// PART 3

                if (filesize > 0) {
                    blockIndex = this->inodes[retVal]->getNextDataBlockIndex();
                    dataStr = this->openData(blockIndex);

                    char dataBlock[512] = {'\0'};
                    char data[512] = {'\0'};

                    //counter++;

                    for (int i = 0; i < filesize; i++) {
                        data[i] = content[(counter * 512) + i];
                        //std::cout << data[i];
                    }

                    this->appendData(dataBlock, 0, data, filesize);
                    this->writeData(blockIndex, dataBlock);
                }

                /*
                INode *inode = this->inodes[retVal];
                int dataBlock2 = inode->getFirstDataBlockIndex();
                std::string retStr = this->mMemblockDevice.readBlock(dataBlock2).toString();
                std::cout << std::endl << "RetSTR: " << retStr << std::endl;

                int nextDataBlock = inode->getNextDataBlockIndex();
                while (nextDataBlock != -1) {
                    std::string retStr = this->mMemblockDevice.readBlock(nextDataBlock).toString();
                    std::cout << "RetSTR: " << retStr << std::endl;

                    nextDataBlock = inode->getNextDataBlockIndex();
                }
                */
            }
        }
    }

    /*
    // old
    if (!filename.empty()) {
        INode *directory = NULL;
        if (found) {
            filepath.erase(i+1, filepath.length());
            //std::cout << "FilePath: " << filepath << std::endl;
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


        if (!isDir) {
            //SKAPA DATA
            //content.append('\0');
            int filesize = content.length();

            int nrOfBlocks = filesize / 512;
            if (nrOfBlocks*512 < filesize || nrOfBlocks == 0)
                nrOfBlocks++;

            if (nrOfBlocks > 10) {
                throw "Error: data to large";
            }

            int* freeData = new int[nrOfBlocks];

            bool exist = true;
            for(int i = 0; i < nrOfBlocks; i++) {
                freeData[i] = this->findNextFreeData();
                if(freeData[i] == -1)
                    exist = false;
            }

            if(!exist) {
                delete this->inodes[retVal];
                this->inodes[retVal] = NULL;
                this->bitmapINodes[retVal] = false;
                throw "Error: no free spaces on harddrive.";
            }

            for(int i = 0; i < nrOfBlocks; i++) {
                this->bitmapData[freeData[i]] = true;
            }

            this->inodes[retVal]->setFilesize(filesize);

            for (int i = 0; i < nrOfBlocks; i++) {
                this->inodes[retVal]->setSpecificDataBlock(i, freeData[i]);
            }

            if (filesize <= 512) {
                std::string dataStr;
                dataStr = this->openData(this->inodes[retVal]->getFirstDataBlockIndex());

                char dataBlock[512] = {'\0'};
                char data[512];

                for (int i = 0; i < filesize; i++) {
                    data[i] = content[i];
                }

                std::cout << "Filesize: " << filesize << std::endl;
                this->appendData(dataBlock, 0, data, filesize);
                this->writeData(this->inodes[retVal]->getFirstDataBlockIndex(), dataBlock);
            } else {
                ///PART 1

                std::string dataStr;
                dataStr = this->openData(this->inodes[retVal]->getFirstDataBlockIndex());

                char dataBlock[512] = {'\0'};
                char data[512];

                for (int i = 0; i < 512; i++) {
                    data[i] = content[i];
                }

                this->appendData(dataBlock, 0, data, 512);
                this->writeData(this->inodes[retVal]->getFirstDataBlockIndex(), dataBlock);
                filesize -= 512;

                /// PART 2

                //int blockIndex = this->inodes[retVal]->getNextDataBlockIndex();
                while (filesize > 512) {
                    int blockIndex = this->inodes[retVal]->getNextDataBlockIndex();
                    dataStr = this->openData(blockIndex);

                    char dataBlock[512] = {'\0'};
                    char data[512];

                    for (int i = 0; i < 512; i++) {
                        data[i] = content[i];
                    }

                    this->appendData(dataBlock, 0, data, 512);
                    this->writeData(this->inodes[retVal]->getFirstDataBlockIndex(), dataBlock);

                    filesize -= 512;

                    //blockIndex = this->inodes[retVal]->getNextDataBlockIndex();
                }

                /// PART 3

                if (filesize > 0) {
                    int blockIndex = this->inodes[retVal]->getNextDataBlockIndex();
                    dataStr = this->openData(blockIndex);

                    char dataBlock[512] = {'\0'};
                    char data[512];

                    for (int i = 0; i < filesize; i++) {
                        data[i] = content[i];
                    }

                    this->appendData(dataBlock, 0, data, filesize);
                    this->writeData(this->inodes[retVal]->getFirstDataBlockIndex(), dataBlock);
                }
            }
        }
    }
    */

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
std::string FileSystem::listDir () {
    std::string retStr = "";

    int* inodesIndexes;
    std::string* directories;

    int nrOfDirs = this->getAllDirectoriesFromDataBlock(this->currentINode, inodesIndexes, directories);


    for (int i = 0; i < nrOfDirs; i++) {
        retStr += this->inodes[inodesIndexes[i]]->toString() + " " + directories[i] + (this->inodes[inodesIndexes[i]]->isDir() ? "/" : "");

        if(i != nrOfDirs-1)
            retStr += "\n";
    }

    delete[] inodesIndexes;
    delete[] directories;

    return retStr;
}

std::string FileSystem::listDir (std::string &filepath) {
    std::string retStr = "";
    INode* location;

    if(filepath == "") {
        location = this->currentINode;
    } else {
        std::string filename;
        bool found = this->splitFilepath(filename, filepath);

        //std::cout << found << std::endl;

        if(found || filepath[0] == '/') {
            int inodeIndex = goToFolder(filepath);
            //std::cout << inodeIndex << std::endl;
            location = this->inodes[inodeIndex];
        } else {
            location = this->currentINode;
        }

        if(filename[0] != '/') {
            int *inodesIndexes;
            std::string *directories;
            int nrOfEntries = this->getAllDirectoriesFromDataBlock(location, inodesIndexes, directories);
            int inodeIndex = this->findInodeIndexByName(directories, nrOfEntries, filename);

            if (!this->inodes[inodesIndexes[inodeIndex]]->isDir()) {
                delete[] inodesIndexes;
                delete[] directories;
                throw "Error: not a folder.";
            }

            location = this->inodes[inodesIndexes[inodeIndex]];
            delete[] inodesIndexes;
            delete[] directories;
        }
    }

    //std::cout << location->isDir() << std::endl;

    int* inodesIndexes;
    std::string* directories;

    int nrOfDirs = this->getAllDirectoriesFromDataBlock(location, inodesIndexes, directories);


    for (int i = 0; i < nrOfDirs; i++) {
        retStr += this->inodes[inodesIndexes[i]]->toString() + " " + directories[i] + (this->inodes[inodesIndexes[i]]->isDir() ? "/" : "");

        if(i != nrOfDirs-1)
            retStr += "\n";
    }

    delete[] inodesIndexes;
    delete[] directories;

    return retStr;
}

std::string FileSystem::openData(int blockIndex) {
    //std::cout << blockIndex << std::endl;
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
int FileSystem::moveToFolder() {
    this->currentINode = this->inodes[0];
    return 0;
}

int FileSystem::moveToFolder(const std::string filepath) {
    int inodeIndex = this->goToFolder(filepath);
    if (inodeIndex != -1) {
        this->currentINode = this->inodes[inodeIndex];
    }
    return inodeIndex;
}

std::string FileSystem::cat(std::string &filepath) {
    //std::cout << "Filepath: " << filepath << std::endl;

    std::string content;
    char* actualData = NULL;

    std::string filename;
    INode* location = NULL;
    INode* final = NULL;
    bool found = this->splitFilepath (filename, filepath);

    int inodeIndex;
    if(!found) {
        location = this->currentINode;
    } else {
        inodeIndex = goToFolder(filepath);
        if(inodeIndex == -1)
            throw "Error: no such directory.";
        location = this->inodes[inodeIndex];
    }

    int* inodesIndexes;
    std::string* directories;

    int nrOfEntries = this->getAllDirectoriesFromDataBlock(location, inodesIndexes, directories);

    /*for(int i = 0; i < nrOfEntries; i++) {
        std::cout << directories[i] << std::endl;
    }*/
    inodeIndex = this->findInodeIndexByName(directories, nrOfEntries, filename);

    if(inodeIndex == -1) {
        delete[] inodesIndexes;
        delete[] directories;
        throw "Error: no such file or directory.";
    }
    final = this->inodes[inodesIndexes[inodeIndex]];

    delete[] inodesIndexes;
    delete[] directories;

    int filesize = final->getFilesize();
    //std::cout << "CatFileSize: " << filesize << std::endl;

    if(filesize == 0)
        throw "File i empty";

    //std::cout << "Name: " << final->getFilename() << std::endl;
    //std::cout << "Filesize: " << filesize << std::endl;

    int blockIndex = final->getFirstDataBlockIndex();
    //std::cout << blockIndex << std::endl;
    content += this->openData(blockIndex);

    blockIndex = final->getNextDataBlockIndex();
    while (blockIndex != -1) {
        content += this->openData(blockIndex);
        blockIndex = final->getNextDataBlockIndex();
    }

    //content.append('\0');
    actualData = new char[filesize];

    for (int i = 0; i < filesize; i++) {
        actualData[i] = content[i];
    }

    actualData[filesize] = '\0';

    return actualData;
}

int FileSystem::removeFolderEntry (INode* inode, std::string filename) {
    int deleted = -1;

    int *inodeIndexes;
    std::string *directories;
    int nrOfDirectories = this->getAllDirectoriesFromDataBlock(inode, inodeIndexes, directories);
    int inodeIndex = findInodeByName(inodeIndexes, directories, nrOfDirectories, filename);
    int entryIndex = findInodeIndexByName(directories, nrOfDirectories, filename);

    //std::cout << "NrOfDirectories: " << nrOfDirectories << std::endl;
    //std::cout << "entryIndex: " << entryIndex << std::endl;

    if(inodeIndex != -1) {

        bool okToDelete = true;
        if (this->inodes[inodeIndex]->isDir()) {
            int *tmpInodeIndexes;
            std::string *tmpDirs;
            int nrOfFilesInDir = this->getAllDirectoriesFromDataBlock(this->inodes[inodeIndex], tmpInodeIndexes,
                                                                      tmpDirs);
            delete[] tmpInodeIndexes;
            delete[] tmpDirs;

            if (nrOfFilesInDir > 2)
                okToDelete = false;
        }

        if (entryIndex != -1 && okToDelete) {
            char dataBlock[512];
            inode->setFilesize(0);
            for (int i = 0; i < nrOfDirectories; i++) {
                if (i != entryIndex) {
                    char tmpDir[16] = {'\0'};
                    tmpDir[0] = (char) inodeIndexes[i];
                    for (int n = 0; n < 15 && directories[i][n] != '\0'; n++) {
                        tmpDir[n + 1] = directories[i][n];
                    }

                    this->appendData(dataBlock, inode->getFilesize(), tmpDir, 16);
                    inode->setFilesize(inode->getFilesize() + 16);
                }
            }

            //TODO delete here

            this->writeData(inode->getFirstDataBlockIndex(), dataBlock);

            deleted = inodeIndex;
        }
    }

    delete[] inodeIndexes;
    delete[] directories;

    return deleted;
}

bool FileSystem::removeFile(std::string filepath) {
    bool retVal = false;

    INode* location;

    if(filepath == "") {
        throw "Error: enter a file or folder.";
    } else {
        std::string filename;
        bool found = this->splitFilepath(filename, filepath);

        //std::cout << found << std::endl;

        if(found || filepath[0] == '/') {
            int inodeIndex = goToFolder(filepath);
            //std::cout << inodeIndex << std::endl;
            if(inodeIndex == -1)
                throw "Error!!!";
            location = this->inodes[inodeIndex];
        } else {
            location = this->currentINode;
        }

        int deletedInode = this->removeFolderEntry(location, filename);

        if(deletedInode != -1) {
            this->bitmapINodes[deletedInode] = false;
            delete this->inodes[deletedInode];
            this->inodes[deletedInode] = NULL;

            retVal = true;
        } else {
            throw "Error: delete rejected. Folder must be empty or file does not exist.";
        }
    }

    return retVal;
}

bool FileSystem::splitFilepath (std::string &filename, std::string &path) {
    bool found = false;
    int i = (int)(path.length() - 1);
    for (; i > 0 && !found; i--) {
        if (path[i] == '/') {
            found = true;
        }
    }

    int n = i;
    if(found) {
        n = i+2;
    }
    filename = path.substr((n), path.length());

    if (!filename.empty()) {

        if (found) {
            path.erase(i + 1, path.length());
        }
    }

    return found;
}

int FileSystem::move (std::string from, std::string to) {
    std::string fromFilename;
    std::string toFilename;

    bool fromFound = splitFilepath (fromFilename, from);
    bool toFound = splitFilepath (toFilename, to);

    INode* fromInode;
    INode* toInode;

    if(!fromFound) {
        //std::cout << "From: Samma mapp" << std::endl;
        fromInode = this->currentINode;
    } else {
        int inodeIndexFrom = goToFolder(from);
        fromInode = this->inodes[inodeIndexFrom];
    }

    if(!toFound) {
        //std::cout << "To: Samma mapp" << std::endl;
        toInode = this->currentINode;
    } else {
        int inodeIndexTo = goToFolder(to);
        toInode = this->inodes[inodeIndexTo];
    }

    int inodeIndex = this->fileExists(toInode, toFilename);
    if (inodeIndex != -1) {
        throw "Error: file exists.";
    }

    // Removed from entry and rename
    int deletedInode = this->removeFolderEntry(fromInode, fromFilename);
    this->inodes[deletedInode]->setFilename(toFilename);

    // Add in other folder
    char data[16] = {'\0'};
    data[0] = (char) deletedInode;
    for(int i = 0; i < 15; i++) {
        data[i+1] = toFilename[i];
    }

    int filesize = toInode->getFilesize();
    int iDataBlock = toInode->getFirstDataBlockIndex();
    std::string dataBlock = this->openData(iDataBlock);

    char cDataBlock[512];
    for (int i = 0; i < 512; i++) {
        cDataBlock[i] = dataBlock[i];
    }

    this->appendData(cDataBlock, filesize ,data, 16);
    toInode->setFilesize(filesize + 16);

    this->writeData(iDataBlock, cDataBlock);

    /*std::cout << from << " from filename: " << fromFilename << std::endl;
    std::cout << to << " to filename: " << toFilename << std::endl;*/
}

int FileSystem::copy (std::string from, std::string to){
    std::string fromFilename;
    std::string toFilename;

    bool fromFound = splitFilepath (fromFilename, from);
    bool toFound = splitFilepath (toFilename, to);

    INode* fromInode;
    INode* toInode;

    int inodeIndexFrom;
    int inodeIndexTo;

    if(!fromFound) {
        std::cout << "From: Samma mapp" << std::endl;
        fromInode = this->currentINode;
    } else {
        inodeIndexFrom = goToFolder(from);
        fromInode = this->inodes[inodeIndexFrom];
    }

    if(!toFound) {
        std::cout << "To: Samma mapp" << std::endl;
        toInode = this->currentINode;
    } else {
        inodeIndexTo = goToFolder(to);
        toInode = this->inodes[inodeIndexTo];
    }

    int* inodesIndex;
    std::string* directories;
    int nrOfEntries = this->getAllDirectoriesFromDataBlock(fromInode, inodesIndex, directories);
    int inodeIndex = this->findInodeIndexByName(directories, nrOfEntries, fromFilename);

    if(inodeIndex == -1) {
        delete[] inodesIndex;
        delete[] directories;
        throw "Error: no such file.";
    }

    // Get copy of inode
    fromInode = this->inodes[inodesIndex[inodeIndex]];
    int filesize = fromInode->getFilesize();

    // Figure out how many datablocks needed
    int nrOfBlocksNeeded = filesize / 512;
    if((nrOfBlocksNeeded*512) < filesize || nrOfBlocksNeeded == 0)
        nrOfBlocksNeeded++;

    int freeInode = this->findNextFreeInode();
    int* freeData = new int[nrOfBlocksNeeded];

    bool exist = true;
    for(int i = 0; i < nrOfBlocksNeeded; i++) {
        freeData[i] = this->findNextFreeData();
        if(freeData[i] == -1)
            exist = false;
    }

    if(freeInode == -1 || !exist) {
        throw "Error: no free spaces on harddrive.";
    }

    int* toInodesIndexes;
    std::string* toDirectories;
    int toNrOfEntries = this->getAllDirectoriesFromDataBlock(toInode, toInodesIndexes, toDirectories);
    int toInodeIndex = this->findInodeIndexByName(toDirectories, toNrOfEntries, toFilename);

    delete[] toInodesIndexes;
    delete[] toDirectories;

    if (toInodeIndex != -1) {
        throw "Error: filename exists.";
    }

    // No errors - set bitmap to taken
    this->bitmapINodes[freeInode] = true;

    for(int i = 0; i < nrOfBlocksNeeded; i++) {
        this->bitmapData[freeData[i]] = true;
    }

    // Copy inode
    this->inodes[freeInode] = new INode(*fromInode);
    this->inodes[freeInode]->setFilename(toFilename);

    for (int i = 0; i < nrOfBlocksNeeded; i++) {
        this->inodes[freeInode]->setSpecificDataBlock(i, freeData[i]);
    }

    // Copy all dataBlocks
    int test = fromInode->getFirstDataBlockIndex();
    //std::cout << "Test: " << test << std::endl;
    //std::cout << "Test: " << test << std::endl;
    std::string dataBlock = this->openData(test);

    char cDataBlock[512];
    for (int i = 0; i < 512; i++) {
        cDataBlock[i] = dataBlock[i];
    }
    this->writeData(this->inodes[freeInode]->getFirstDataBlockIndex(), cDataBlock);

    int iDataBlockFrom = fromInode->getNextDataBlockIndex();
    while(iDataBlockFrom != -1) {
        dataBlock = this->openData(iDataBlockFrom);
        char cDataBlock[512];
        for (int i = 0; i < 512; i++) {
            cDataBlock[i] = dataBlock[i];
        }
        this->writeData(this->inodes[freeInode]->getNextDataBlockIndex(), cDataBlock);

        iDataBlockFrom = fromInode->getNextDataBlockIndex();
    }

    // Add inodeEntry to parent
    char cFilename[16] = {'\0'};
    cFilename[0] = (char) freeInode;
    for (int i = 0; i < 15; i++) {
        cFilename[i + 1] = toFilename[i];
    }

    std::string toDataBlock = this->openData(toInode->getFirstDataBlockIndex());
    char cToDataBlock[512];
    for(int i = 0; i < 512; i++) {
        cToDataBlock[i] = toDataBlock[i];
    }
    this->appendData(cToDataBlock, toInode->getFilesize(), cFilename, 16);
    this->writeData(toInode->getFirstDataBlockIndex(), cToDataBlock);
    toInode->setFilesize(toInode->getFilesize()+16);

    /*std::cout << from << " from filename: " << fromFilename << std::endl;
    std::cout << to << " to filename: " << toFilename << std::endl;*/
}

int FileSystem::appendFile(std::string to, std::string from) {

    std::string fromFilename;
    std::string toFilename;

    bool fromFound = splitFilepath (fromFilename, from);
    bool toFound = splitFilepath (toFilename, to);


    INode* fromInode;
    INode* toInode;

    int inodeIndexFrom;
    int inodeIndexTo;

    if(!fromFound) {
        //std::cout << "From: Samma mapp" << std::endl;
        fromInode = this->currentINode;
    } else {
        inodeIndexFrom = goToFolder(from);
        fromInode = this->inodes[inodeIndexFrom];
    }

    if(!toFound) {
        //std::cout << "To: Samma mapp" << std::endl;
        toInode = this->currentINode;
    } else {
        inodeIndexTo = goToFolder(to);
        toInode = this->inodes[inodeIndexTo];
    }


    int fromINodeIndex = this->fileExists(fromInode, fromFilename);
    int toINodeIndex = this->fileExists(toInode, toFilename);

    if (fromINodeIndex == -1 || toINodeIndex == -1) {
        throw "Error file does'nt exists.";
    }

    // Nytt
    int *inodeIndexes;
    std::string *directories;
    int nrOfDirs = this->getAllDirectoriesFromDataBlock(fromInode, inodeIndexes, directories);
    int inodeIndex = this->findInodeIndexByName(directories, nrOfDirs, fromFilename);

    fromInode = this->inodes[inodeIndexes[inodeIndex]];

    inodeIndexes = NULL;
    directories = NULL;
    nrOfDirs = this->getAllDirectoriesFromDataBlock(toInode, inodeIndexes, directories);
    inodeIndex = this->findInodeIndexByName(directories, nrOfDirs, toFilename);

    toInode = this->inodes[inodeIndexes[inodeIndex]];

    delete[] inodeIndexes;
    delete[] directories;
    //Slut på nytt

    //fromInode = this->inodes[fromINodeIndex];
    //toInode = this->inodes[toINodeIndex];

    /*int blockIndex = toInode->getFirstDataBlockIndex();
    std::cout << "BlockIndex: " << blockIndex << std::endl;
    blockIndex = toInode->getNextDataBlockIndex();

    while (blockIndex != -1) {
        std::cout << "BlockIndex: " << blockIndex << std::endl;
        blockIndex = toInode->getNextDataBlockIndex();
    }*/

    if (fromInode->isDir() || toInode->isDir()) {
        throw "Error: can only append to files.";
    }

    int fromFileSize = fromInode->getFilesize();
    int toFileSize = toInode->getFilesize();

    if (fromFileSize + toFileSize > INode::MAX_FILESIZE) {
        throw "Error: file's to big to fit inside.";
    }

    int fromBlockIndex = fromInode->getFirstDataBlockIndex();
    std::string fromContent = this->openData(fromBlockIndex);

    fromBlockIndex = fromInode->getNextDataBlockIndex();
    while (fromBlockIndex != -1) {
        fromContent += this->openData(fromBlockIndex);
        fromBlockIndex = fromInode->getNextDataBlockIndex();
    }

    fromContent = fromContent.substr(0, fromFileSize);

    int toLastTmpBlockIndex = 0;

    int toLastBlockIndex = toLastTmpBlockIndex = toInode->getFirstDataBlockIndex();

    toLastBlockIndex = toInode->getNextDataBlockIndex();
    int toBlockCounter = 1;
    while (toLastBlockIndex != -1) {
        toLastTmpBlockIndex = toLastBlockIndex;
        toLastBlockIndex = toInode->getNextDataBlockIndex();
        toBlockCounter++;
    }

    int tmpToFileSize = toFileSize;

    for (int i = 0; i < toBlockCounter - 1; i++) {
        tmpToFileSize -= 512;
    }

    int byteRemainingTo = 512 - tmpToFileSize;
    int tmpFromFileSize = fromFileSize - byteRemainingTo;

    int nrOfBlocksNeeded = tmpFromFileSize / 512;
    if (nrOfBlocksNeeded * 512 < tmpFromFileSize) {
        nrOfBlocksNeeded++;
    }

    int *freeDataBlocks = new int[nrOfBlocksNeeded];
    bool freeFound = true;
    for (int i = 0; i < nrOfBlocksNeeded; i++) {
        freeDataBlocks[i] = this->findNextFreeData();
        if (freeDataBlocks[i] == -1) {
            freeFound = false;
        }
    }

    if (!freeFound) {
        throw "Error: no free data blocks.";
    }

    char cToLastBlockData[512];
    std::string toLastBlockData = this->openData(toLastTmpBlockIndex);
    for (int i = 0; i < 512; i++) {
        cToLastBlockData[i] = toLastBlockData[i];
    }

    for (int i = tmpToFileSize; i < 512; i++) {
        cToLastBlockData[i] = fromContent[i - tmpToFileSize];
    }

    this->writeData(toLastTmpBlockIndex, cToLastBlockData);

    int tmpBlockIndex = toBlockCounter;
    for (int i = 0; i < nrOfBlocksNeeded; i++) {
        //std::cout << "toBlockCounter: " << toBlockCounter << std::endl;
        toInode->setSpecificDataBlock(tmpBlockIndex++, freeDataBlocks[i]);
        //std::cout << "freeDataBlocks[" << i << "]: " << freeDataBlocks[i] << std::endl;
    }
    //std::cout << "toBlockCounter: " << toBlockCounter << std::endl;

    int newBlockIndex = toLastTmpBlockIndex + 1;
    //std::cout << "newBlockIndex: " << newBlockIndex << std::endl;
    int startFrom = byteRemainingTo;
    while (newBlockIndex != -1) {
        char cDataBlock[512];
        //std::string dataBlockStr = this->openData(newBlockIndex);
        for (int i = 0; i < 512; i++) {
            cDataBlock[i] = fromContent[startFrom + i];
        }

        this->writeData(newBlockIndex, cDataBlock);

        startFrom += 512;
        newBlockIndex = toInode->getNextDataBlockIndex();
    }

    toInode->setFilesize(toInode->getFilesize() + fromInode->getFilesize());

    delete[] freeDataBlocks;
    freeDataBlocks = NULL;

    /*blockIndex = toInode->getFirstDataBlockIndex();
    std::cout << "LastBlockIndex: " << blockIndex << std::endl;
    std::cout << this->openData(blockIndex) << std::endl;

    blockIndex = toInode->getNextDataBlockIndex();

    while (blockIndex != -1) {
        std::cout << "LastBlockIndex: " << blockIndex << std::endl;
        std::cout << this->openData(blockIndex) << std::endl;

        blockIndex = toInode->getNextDataBlockIndex();
    }*/

    return 0;
}
