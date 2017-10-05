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
    std::cout << blockIndex << std::endl;
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

    if(filesize == 0)
        throw "File i empty";

    //std::cout << "Name: " << final->getFilename() << std::endl;
    //std::cout << "Filesize: " << filesize << std::endl;

    int blockIndex = final->getFirstDataBlockIndex();
    content += this->openData(blockIndex);

    blockIndex = final->getNextDataBlockIndex();
    while (blockIndex != -1) {
        content += this->openData(blockIndex);
        blockIndex = final->getNextDataBlockIndex();
    }

    actualData = new char[filesize];
    for (int i = 0; i < filesize; i++) {
        actualData[i] = content[i];
    }

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

    int* inodesIndex;
    std::string* directories;
    int nrOfEntries = this->getAllDirectoriesFromDataBlock(fromInode, inodesIndex, directories);
    int inodeIndex = this->findInodeIndexByName(directories, nrOfEntries, fromFilename);

    if(inodeIndex == -1) {
        delete[] inodesIndex;
        delete[] directories;
        throw "Error: no such file.";
    }

    // Copy inode
    fromInode = this->inodes[inodesIndex[inodeIndex]];
    int filesize = fromInode->getFilesize();

    int nrOfBlocksNeeded = filesize / 512;
    if((nrOfBlocksNeeded*512) < filesize)
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
    std::string dataBlock = this->openData(fromInode->getFirstDataBlockIndex());
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
