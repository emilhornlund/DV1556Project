#include "filesystem.h"
#include <iostream>
#include <fstream>



/* CONSTRUCTORS ETC */
FileSystem::FileSystem() {
    this->currentINode = NULL;
    this->_init();
}

FileSystem::~FileSystem() {
    this->_reset();
}



/* PRIVATE FUNCTIONS */
void FileSystem::_init() {
    for (int i = 0; i < MemBlockDevice::MEM_SIZE; i++) {
        this->inodes[i] = NULL;
        this->bitmapData[i] = false;
        this->bitmapINodes[i] = false;
    }

    this->bitmapDataIndex = 0;
    this->bitmapInodeIndex = 0;

    this->currentINode = this->inodes[this->createInode(0, "/", 7, "root", "root", "/", true, false)];
}

void FileSystem::format () {
    this->_reset();
    this->_init();
}

int FileSystem::_findNextFreeData() {
    int retVal = -1;
    int stopIndex = (this->bitmapDataIndex - 1) % MemBlockDevice::MEM_SIZE;
    bool found = false;

    for (; this->bitmapDataIndex != stopIndex && !found; this->bitmapDataIndex++) {
        if (this->bitmapDataIndex >= MemBlockDevice::MEM_SIZE) this->bitmapDataIndex = 0;

        if (!this->bitmapData[this->bitmapDataIndex]) {
            retVal = this->bitmapDataIndex;
            found = true;
        }
    }

    return retVal;
}

int FileSystem::_findNextFreeInode() {
    int retVal = -1;
    int stopIndex = (this->bitmapInodeIndex - 1) % MemBlockDevice::MEM_SIZE;
    bool found = false;

    for (; this->bitmapInodeIndex != stopIndex && !found; this->bitmapInodeIndex++) {
        if (this->bitmapInodeIndex >= MemBlockDevice::MEM_SIZE) this->bitmapInodeIndex = 0;

        if (!this->bitmapINodes[this->bitmapInodeIndex]) {
            retVal = this->bitmapInodeIndex;
            found = true;
        }
    }

    return retVal;
}

int FileSystem::_findInodeByName(int *inodeIndexes, std::string *filenames, int size, std::string searchFilename) {
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

int FileSystem::_findInodeIndexByName(std::string *filenames, int size, std::string searchFilename) {
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

void FileSystem::_resetINodes() {
	for (int i = 0; i < MemBlockDevice::MEM_SIZE; i++) {
		if (this->inodes[i] != NULL) {
			delete this->inodes[i];
			this->inodes[i] = NULL;
		}
	}
	this->currentINode = NULL;
}

void FileSystem::_resetBitmapINodes() {
	for (int i = 0; i < MemBlockDevice::MEM_SIZE; i++)
		this->bitmapINodes[i] = false;
}

void FileSystem::_resetBitmapData() {
	for (int i = 0; i < MemBlockDevice::MEM_SIZE; i++)
		this->bitmapData[i] = false;
}

void FileSystem::_reset() {
    this->_resetINodes();
    this->_resetBitmapINodes();
    this->_resetBitmapData();
}

int FileSystem::_fileExists(INode *inode, std::string filename) {
    int *inodeIndexes;
    std::string *directories;
    int nrOfDirs = this->_getAllDirectoriesFromDataBlock(inode, inodeIndexes, directories);
    int inodeIndex = this->_findInodeIndexByName(directories, nrOfDirs, filename);

    delete[] inodeIndexes;
    delete[] directories;

    return inodeIndex;
}

void FileSystem::_checkPermissions(int permission, bool *&permissions) {
    permissions = new bool[3];
    for(int i = 0; i < 3; i++)
        permissions[i] = NULL;

    if ((permission - 4) >= 0) {
        permissions[0] = true; //READ
        permission -= 4;
    } else {
        permissions[0] = false;
    }

    if ((permission - 2) >= 0) {
        permissions[1] = true; //WRITE
        permission -= 2;
    } else {
        permissions[1] = false;
    }

    if ((permission - 1) >= 0) {
        permissions[2] = true; //EXECUTE
    } else {
        permissions[2] = false;
    }
}

int FileSystem::_appendData (char* dataBlock, int currentBlockSize, const char* data, int dataSize) {
    int iLow = 0;

    for (int i = currentBlockSize; i < (currentBlockSize + dataSize); i++) {
        dataBlock[i] = data[iLow];
        iLow++;
    }

    return dataSize;
}

void FileSystem::_writeData(int dataBlock, char *data) {
    this->mMemblockDevice.writeBlock(dataBlock, data);
}

int FileSystem::_getAllDirectoriesFromDataBlock(INode *inode, int *&inodes, std::string *&directories) {
    int nrOfDirs = inode->getFilesize()/16;
    int dataBlockIndex = inode->getFirstDataBlockIndex();
    directories = NULL;
    inodes      = NULL;

    if (dataBlockIndex != -1) {
        directories = new std::string[nrOfDirs];
        inodes      = new int[nrOfDirs];

        std::string dataBlock = this->_openData(dataBlockIndex);

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

std::string FileSystem::_openData(int blockIndex) {
    if(blockIndex < 0 || blockIndex >= Block::BLOCK_SIZE) {
        throw std::out_of_range("Exception: out of range");
    }
    return this->mMemblockDevice.readBlock(blockIndex).toString();
}

int FileSystem::_goToFolder(std::string &filePath) {
    int inodeIndex = -2;
    INode* startFromInode = this->currentINode;                 // Relative path

    if (filePath[0] == '/') {                                   // Absolute path - Start from root (0)
        startFromInode = this->inodes[0];
        filePath.erase(0, 1);
    }

    std::vector<std::string> names;

    if (filePath.length() > 0) {
        std::string tmpStr;
        for (unsigned int i = 0; i < filePath.length(); i++) {
            if (filePath[i] != '/') {
                tmpStr += filePath[i];
            } else {
                names.push_back(tmpStr);
                tmpStr = "";
            }
        }

        names.push_back(tmpStr);

        unsigned int i = 0;
        for (; i < names.size(); i++) {
            int *inodeIndexes;
            std::string *directories;
            int nrOfDirectories = this->_getAllDirectoriesFromDataBlock(startFromInode, inodeIndexes, directories);

            inodeIndex = _findInodeByName(inodeIndexes, directories, nrOfDirectories, names[i]);

            delete[] inodeIndexes;
            delete[] directories;

            if (inodeIndex != -1 && this->inodes[inodeIndex]->isDir()) {
                bool *permissions;
                this->_checkPermissions(this->inodes[inodeIndex]->getProtection(), permissions);

                if (!permissions[0])
                    throw "Error: permission denied.";

                delete[] permissions;

                startFromInode = this->inodes[inodeIndex];
                if (!startFromInode->isDir()) {
                    throw "Not a directory";
                }
            } else {
                inodeIndex = -1;
                break;
            }
        }

    }

    if(startFromInode->getThisInodeIndex() == 0 && inodeIndex == -2)
        inodeIndex = 0;

    return inodeIndex;
}

int FileSystem::_removeFolderEntry(INode *inode, std::string filename) {
    int deleted = -1;

    int *inodeIndexes;
    std::string *directories;
    int nrOfDirectories = this->_getAllDirectoriesFromDataBlock(inode, inodeIndexes, directories);
    int inodeIndex = _findInodeByName(inodeIndexes, directories, nrOfDirectories, filename);
    int entryIndex = _findInodeIndexByName(directories, nrOfDirectories, filename);

    if(inodeIndex != -1) {

        bool* permissions;
        this->_checkPermissions(this->inodes[inodeIndex]->getProtection(), permissions);

        if(!permissions[1]) {
            delete[] inodeIndexes;
            delete[] directories;
            throw "Error: permission denied.";
        }

        delete[] permissions;

        bool okToDelete = true;
        if (this->inodes[inodeIndex]->isDir()) {
            int *tmpInodeIndexes;
            std::string *tmpDirs;
            int nrOfFilesInDir = this->_getAllDirectoriesFromDataBlock(this->inodes[inodeIndex], tmpInodeIndexes,
                                                                       tmpDirs);
            delete[] tmpInodeIndexes;
            delete[] tmpDirs;

            if (nrOfFilesInDir > 2)
                okToDelete = false;
        }

        if (entryIndex != -1 && okToDelete) {
            char dataBlock[Block::BLOCK_SIZE];
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

            this->_writeData(inode->getFirstDataBlockIndex(), dataBlock);
            deleted = inodeIndex;
        }
    }

    delete[] inodeIndexes;
    delete[] directories;

    return deleted;
}

bool FileSystem::_splitFilepath(std::string &filename, std::string &path) {
    bool found = false;
    int i = (int)(path.length() - 1);
    for (; i > 0 && !found; i--) {
        if (path[i] == '/') {
            found = true;
        }
    }

    int n = i;
    if(found)
        n = i+2;
    filename = path.substr((n), path.length());

    if (!filename.empty()) {
        if (found)
            path.erase(i + 1, path.length());
    }

    return found;
}



/* PUBLIC FUNCTIONS */
std::string FileSystem::getPWD() const {
    return this->currentINode->getPWD();
}

int FileSystem::createFile(std::string filepath, std::string username, std::string &content, const bool isDir) {
    int retVal = -1;

    std::string filename;
    bool found = this->_splitFilepath(filename, filepath);

    INode *location;
    if(found || filepath[0] == '/') {
        int inodeIndex = _goToFolder(filepath);
        location = this->inodes[inodeIndex];
    } else {
        location = this->currentINode;
    }

    if (!location->isDir()) throw "create: cannot create: Not a directory";

    bool* permissions;
    this->_checkPermissions(location->getProtection(), permissions);

    if(!permissions[1])
        throw "Error: permission denied.";

    delete[] permissions;

    int inodeIndex = this->_fileExists(location, filename);
    if (inodeIndex != -1) {
        throw "Error: folder/file exists.";
    }

    if (!filename.empty()) {
        std::string newPWD = location->getPWD() + filename;
        if (isDir) {
            newPWD += "/";
        }
        retVal = this->createInode(location->getThisInodeIndex(), filename, 7, username, username, newPWD, isDir, false);

        if(retVal == -1)
            throw "Error: filename too long.";

        if (!isDir) {
            unsigned int filesize = (unsigned int) content.length();

            unsigned int nrOfBlocks = filesize / Block::BLOCK_SIZE;
            if (nrOfBlocks*Block::BLOCK_SIZE < filesize || nrOfBlocks == 0)
                nrOfBlocks++;

            if (nrOfBlocks > INode::DATA_ENTRIES)
                throw "Error: data to large";

            int* freeData = new int[nrOfBlocks];

            bool exist = true;
            for(unsigned int i = 0; i < nrOfBlocks; i++) {
                freeData[i] = this->_findNextFreeData();
                if(freeData[i] == -1)
                    exist = false;
            }

            if(!exist) {
                delete this->inodes[retVal];
                this->inodes[retVal] = NULL;
                this->bitmapINodes[retVal] = false;
                throw "Error: no free spaces on harddrive.";
            }

            for(unsigned int i = 0; i < nrOfBlocks; i++)
                this->bitmapData[freeData[i]] = true;

            this->inodes[retVal]->setFilesize(filesize);
            for (unsigned int i = 0; i < nrOfBlocks; i++)
                this->inodes[retVal]->setSpecificDataBlock(i, freeData[i]);

            delete[] freeData;

            if (filesize <= Block::BLOCK_SIZE) {
                std::string dataStr;
                dataStr = this->_openData(this->inodes[retVal]->getFirstDataBlockIndex());
                this->inodes[retVal]->addBlockIndex();

                char dataBlock[Block::BLOCK_SIZE] = {'\0'};
                char data[Block::BLOCK_SIZE] = {'\0'};

                for (unsigned int i = 0; i < filesize; i++)
                    data[i] = content[i];

                this->appendData(dataBlock, 0, data, filesize);
                this->_writeData(this->inodes[retVal]->getFirstDataBlockIndex(), dataBlock);
            } else {
                ///PART 1
                int blockIndex = this->inodes[retVal]->getFirstDataBlockIndex();
                this->inodes[retVal]->addBlockIndex();

                std::string dataStr;
                dataStr = this->_openData(blockIndex);

                char dataBlock[Block::BLOCK_SIZE] = {'\0'};
                char data[Block::BLOCK_SIZE];

                for (int i = 0; i < Block::BLOCK_SIZE; i++)
                    data[i] = content[i];

                this->appendData(dataBlock, 0, data, Block::BLOCK_SIZE);
                this->_writeData(blockIndex, dataBlock);
                filesize -= Block::BLOCK_SIZE;

                /// PART 2
                int counter = 1;
                while (filesize > Block::BLOCK_SIZE) {
                    blockIndex = this->inodes[retVal]->getNextDataBlockIndex();
                    this->inodes[retVal]->addBlockIndex();
                    dataStr = this->_openData(blockIndex);

                    char dataBlock[Block::BLOCK_SIZE] = {'\0'};
                    char data[Block::BLOCK_SIZE];

                    for (int i = 0; i < Block::BLOCK_SIZE; i++)
                        data[i] = content[(counter * Block::BLOCK_SIZE) + i];

                    this->appendData(dataBlock, 0, data, Block::BLOCK_SIZE);
                    this->_writeData(blockIndex, dataBlock);
                    filesize -= Block::BLOCK_SIZE;

                    counter++;
                }

                /// PART 3
                if (filesize > 0) {
                    blockIndex = this->inodes[retVal]->getNextDataBlockIndex();
                    this->inodes[retVal]->addBlockIndex();
                    dataStr = this->_openData(blockIndex);

                    char dataBlock[Block::BLOCK_SIZE] = {'\0'};
                    char data[Block::BLOCK_SIZE] = {'\0'};

                    for (unsigned int i = 0; i < filesize; i++)
                        data[i] = content[(counter * Block::BLOCK_SIZE) + i];

                    this->appendData(dataBlock, 0, data, filesize);
                    this->_writeData(blockIndex, dataBlock);
                }
            }
        }
    }

    return retVal;
}

int FileSystem::createInode(unsigned int parentInodeIndex, std::string filename, unsigned int protection, std::string creator, std::string owner, std::string pwd, bool isDir, bool isHidden) {
    int inodeIndex = this->_findNextFreeInode();

    if(owner.length() > 10 || creator.length() > 10) {
        throw "Error: username or creatorname too long.";
    }

    if(pwd.length() > Block::BLOCK_SIZE) {
        throw "Error: filepath is too long.";
    }

    if (this->currentINode != NULL) {
        if(filename.length() < 16) {
            INode *parent = this->inodes[parentInodeIndex];

            int tmpFileSize = parent->getFilesize();
            if (tmpFileSize + 16 < Block::BLOCK_SIZE) {
                std::string tmpDataBlock = this->mMemblockDevice.readBlock(parent->getFirstDataBlockIndex()).toString();

                char data[16];
                data[0] = (char)inodeIndex;

                unsigned int i = 0;
                for (; i < filename.length(); i++) {
                    data[(i + 1)] = filename[i];
                }

                if(i < 15) {
                    data[++i] = '\0';
                }

                char* tmpData = new char[tmpDataBlock.size()];
                std::copy(tmpDataBlock.begin(), tmpDataBlock.end(), tmpData);

                this->appendData(tmpData, tmpFileSize, data, 16);

                this->_writeData(parent->getFirstDataBlockIndex(), tmpData);
                parent->setFilesize(parent->getFilesize()+16);
                delete[] tmpData;

            }

        } else {
            //abort
            inodeIndex = -1;
        }
    }

    int dataIndex = this->_findNextFreeData();
    int tmpFileSize = 0;

    if (inodeIndex != -1 && dataIndex != -1) {
        this->bitmapINodes[inodeIndex] = true;
        this->bitmapData[dataIndex] = true;

        char cFilename[15] = { '\0' };
        char cOwner[10] = { '\0' };
        char cCreator[10] = { '\0' };
        char cPwd[512] = { '\0' };

        for(int i = 0; i < 15 && filename[i] != '\0'; i++) {
            cFilename[i] = filename[i];
        }

        for(int i = 0; i < 10 && (owner[i] != '\0' || creator[i] != '\0'); i++) {
            cOwner[i] = owner[i];
            cCreator[i] = creator[i];
        }

        for(int i = 0; i < 512 && pwd[i] != '\0'; i++) {
            cPwd[i] = pwd[i];
        }

        /*cFilename[filename.length()] = '\0';
        cOwner[owner.length()] = '\0';
        cCreator[creator.length()] = '\0';
        cPwd[pwd.length()] = '\0';*/

        this->inodes[inodeIndex] = new INode(parentInodeIndex, inodeIndex, cFilename, protection, cCreator, cOwner, cPwd, tmpFileSize, isDir);
        this->inodes[inodeIndex]->setFilesize(0);
        this->inodes[inodeIndex]->setDataBlock(dataIndex);

        if (isDir) {

            char data[16] = {(char)inodeIndex, '.', '\0'};
            char data2[16] = {(char)parentInodeIndex, '.', '.', '\0'};
            char dataBlock[Block::BLOCK_SIZE];

            appendData(dataBlock, tmpFileSize, data, 16);
            tmpFileSize += 16;

            appendData(dataBlock, tmpFileSize, data2, 16);
            tmpFileSize += 16;

            this->inodes[inodeIndex]->setFilesize(tmpFileSize);
            this->inodes[inodeIndex]->setDataBlock(dataIndex);
            this->inodes[inodeIndex]->addBlockIndex();
            this->_writeData(dataIndex, dataBlock);
        }
    } else {
        inodeIndex = -1;
    }
    return inodeIndex;
}

int FileSystem::appendData (char* dataBlock, int currentBlockSize, const char* data, int dataSize) {
    int sizeRemainingInBlock = Block::BLOCK_SIZE - currentBlockSize;
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

std::string FileSystem::listDir () {
    std::string retStr = "Permission\tSize\tFolder\tOwner\tCreator\tName\n";

    int* inodesIndexes;
    std::string* directories;

    int nrOfDirs = this->_getAllDirectoriesFromDataBlock(this->currentINode, inodesIndexes, directories);

    for (int i = 0; i < nrOfDirs && inodes[inodesIndexes[i]] != NULL; i++) {
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
    bool absolute = false;

    if(filepath == "") {
        location = this->currentINode;
    } else {
        std::string filename;
        bool found = this->_splitFilepath(filename, filepath);

        if(found || filepath[0] == '/') {
            int inodeIndex = _goToFolder(filepath);

            if(inodeIndex == -1)
                throw "Error: invalid path";

            location = this->inodes[inodeIndex];
            if(filename[0] == '/')
                filename.erase(0, 1);
            absolute = true;
        } else {
            location = this->currentINode;
        }

        if(!absolute) {
            int *inodesIndexes;
            std::string *directories;
            int nrOfEntries = this->_getAllDirectoriesFromDataBlock(location, inodesIndexes, directories);
            int inodeIndex = this->_findInodeIndexByName(directories, nrOfEntries, filename);

            if(inodeIndex == -1)
                throw "Error: invalid path";

            if (!this->inodes[inodesIndexes[inodeIndex]]->isDir()) {
                delete[] inodesIndexes;
                delete[] directories;
                throw "Error: not a folder.";
            }

            location = this->inodes[inodesIndexes[inodeIndex]];
            delete[] inodesIndexes;
            delete[] directories;
        } else if (filename != filepath) {
            int *inodesIndexes;
            std::string *directories;
            int nrOfEntries = this->_getAllDirectoriesFromDataBlock(location, inodesIndexes, directories);
            int inodeIndex = this->_findInodeIndexByName(directories, nrOfEntries, filename);

            if(inodeIndex == -1) {
                throw "Error: invalid path";
            }

            location = this->inodes[inodesIndexes[inodeIndex]];

            delete[] inodesIndexes;
            delete[] directories;
        }
    }

    bool* permissions;
    this->_checkPermissions(location->getProtection(), permissions);

    if(!permissions[0])
        throw "Error: permission denied.";

    delete[] permissions;

    int* inodesIndexes;
    std::string* directories;

    int nrOfDirs = this->_getAllDirectoriesFromDataBlock(location, inodesIndexes, directories);

    for (int i = 0; i < nrOfDirs; i++) {
        retStr += this->inodes[inodesIndexes[i]]->toString() + " " + directories[i] + (this->inodes[inodesIndexes[i]]->isDir() ? "/" : "");

        if(i != nrOfDirs-1)
            retStr += "\n";
    }

    delete[] inodesIndexes;
    delete[] directories;

    return retStr;
}

int FileSystem::moveToFolder() {
    this->currentINode = this->inodes[0];
    return 0;
}

int FileSystem::moveToFolder(std::string &filepath) {
    int inodeIndex = this->_goToFolder(filepath);
    if (inodeIndex == -1)
        throw "Error: filepath do not exist.";

    this->currentINode = this->inodes[inodeIndex];

    return inodeIndex;
}

std::string FileSystem::cat(std::string &filepath) {
    std::string content;
    char* actualData = NULL;

    std::string filename;
    INode* location = NULL;
    INode* final = NULL;
    bool found = this->_splitFilepath(filename, filepath);

    int inodeIndex;
    if(!found) {
        location = this->currentINode;
    } else {
        inodeIndex = _goToFolder(filepath);
        if(inodeIndex == -1)
            throw "Error: no such directory.";
        location = this->inodes[inodeIndex];
    }

    int* inodesIndexes;
    std::string* directories;

    int nrOfEntries = this->_getAllDirectoriesFromDataBlock(location, inodesIndexes, directories);

    inodeIndex = this->_findInodeIndexByName(directories, nrOfEntries, filename);

    if(inodeIndex == -1) {
        delete[] inodesIndexes;
        delete[] directories;
        throw "Error: no such file or directory.";
    }
    final = this->inodes[inodesIndexes[inodeIndex]];

    delete[] inodesIndexes;
    delete[] directories;

    bool* permissions;
    this->_checkPermissions(final->getProtection(), permissions);

    if(!permissions[0])
        throw "Error: permission denied.";

    delete[] permissions;

    int filesize = final->getFilesize();

    if(filesize == 0)
        throw "File i empty";

    int blockIndex = final->getFirstDataBlockIndex();
    content += this->_openData(blockIndex);

    blockIndex = final->getNextDataBlockIndex();
    while (blockIndex != -1) {
        content += this->_openData(blockIndex);
        blockIndex = final->getNextDataBlockIndex();
    }

    actualData = new char[filesize];
    for (int i = 0; i < filesize; i++) {
        actualData[i] = content[i];
    }

    actualData[filesize] = '\0';
    return actualData;
}

bool FileSystem::removeFile(std::string filepath) {
    bool retVal = false;

    INode* location;

    if(filepath == "") {
        throw "Error: enter a file or folder.";
    } else {
        std::string filename;
        bool found = this->_splitFilepath(filename, filepath);

        if(found || filepath[0] == '/') {
            int inodeIndex = _goToFolder(filepath);

            if(inodeIndex == -1)
                throw "Error!!!";
            location = this->inodes[inodeIndex];
        } else {
            location = this->currentINode;
        }

        int deletedInode = this->_removeFolderEntry(location, filename);

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

void FileSystem::move (std::string from, std::string to) {
    std::string fromFilename;
    std::string toFilename;

    bool fromFound = _splitFilepath(fromFilename, from);
    bool toFound = _splitFilepath(toFilename, to);

    if(fromFilename.length() > 15 || toFilename.length() > 15) {
        throw "Error: filename too long";
    }

    INode* fromInode;
    INode* toInode;

    if(!fromFound) {
        fromInode = this->currentINode;
    } else {
        int inodeIndexFrom = _goToFolder(from);
        fromInode = this->inodes[inodeIndexFrom];
    }

    if(!toFound) {
        toInode = this->currentINode;
    } else {
        int inodeIndexTo = _goToFolder(to);
        toInode = this->inodes[inodeIndexTo];
    }

    int inodeIndex = this->_fileExists(toInode, toFilename);
    if (inodeIndex != -1)
        throw "Error: file exists.";

    int* inodeIndexes;
    std::string* directories;
    int nrOfDirs = this->_getAllDirectoriesFromDataBlock(fromInode, inodeIndexes, directories);
    int indexInode = this->_findInodeIndexByName(directories, nrOfDirs, fromFilename);

    if(indexInode == -1) {
        delete[] inodeIndexes;
        delete[] directories;
        throw "Error: file not found.";
    }

    bool* permissions;
    this->_checkPermissions(toInode->getProtection(), permissions);

    if(!permissions[1]) {
        delete[] inodeIndexes;
        delete[] directories;
        throw "Error: permission denied.";
    }

    delete[] permissions;

    this->_checkPermissions(this->inodes[inodeIndexes[indexInode]]->getProtection(), permissions);
    delete[] inodeIndexes;
    delete[] directories;

    if(!permissions[1])
        throw "Error: permission denied.";

    delete[] permissions;

    // Removed from entry and rename
    int deletedInode = this->_removeFolderEntry(fromInode, fromFilename);
    char cToFilename[15];
    for (int i = 0; i < 15; i++)
        cToFilename[i] = toFilename[i];
    this->inodes[deletedInode]->setFilename(cToFilename);

    // Add in other folder
    char data[16] = {'\0'};
    data[0] = (char) deletedInode;
    for(int i = 0; i < 15; i++)
        data[i+1] = toFilename[i];

    int filesize = toInode->getFilesize();
    int iDataBlock = toInode->getFirstDataBlockIndex();
    std::string dataBlock = this->_openData(iDataBlock);
    char cDataBlock[Block::BLOCK_SIZE];
    for (int i = 0; i < Block::BLOCK_SIZE; i++)
        cDataBlock[i] = dataBlock[i];

    this->appendData(cDataBlock, filesize ,data, 16);
    toInode->setFilesize(filesize + 16);
    this->_writeData(iDataBlock, cDataBlock);
}

void FileSystem::copy (std::string from, std::string to){
    std::string fromFilename;
    std::string toFilename;

    bool fromFound = _splitFilepath(fromFilename, from);
    bool toFound = _splitFilepath(toFilename, to);

    if(fromFilename.length() > 15 || toFilename.length() > 15) {
        throw "Error: filename too long";
    }

    INode* fromInode;
    INode* toInode;

    int inodeIndexFrom;
    int inodeIndexTo;

    if(!fromFound) {
        fromInode = this->currentINode;
    } else {
        inodeIndexFrom = _goToFolder(from);
        fromInode = this->inodes[inodeIndexFrom];
    }

    if(!toFound) {
        toInode = this->currentINode;
    } else {
        inodeIndexTo = _goToFolder(to);
        toInode = this->inodes[inodeIndexTo];
    }

    int* inodeIndexes;
    std::string* directories;
    int nrOfEntries = this->_getAllDirectoriesFromDataBlock(fromInode, inodeIndexes, directories);
    int inodeIndex = this->_findInodeIndexByName(directories, nrOfEntries, fromFilename);

    if(inodeIndex == -1) {
        delete[] inodeIndexes;
        delete[] directories;
        throw "Error: no such file.";
    }

    bool* permissions;
    this->_checkPermissions(this->inodes[inodeIndexes[inodeIndex]]->getProtection(), permissions);

    if(!permissions[0] || !permissions[1]) {
        delete[] inodeIndexes;
        delete[] directories;
        throw "Error: permission denied.";
    }

    delete[] permissions;

    this->_checkPermissions(toInode->getProtection(), permissions);

    if(!permissions[0] || !permissions[1]) {
        delete[] inodeIndexes;
        delete[] directories;
        throw "Error: permission denied.";
    }

    delete[] permissions;

    // Get copy of inode
    fromInode = this->inodes[inodeIndexes[inodeIndex]];
    int filesize = fromInode->getFilesize();

    delete[] inodeIndexes;
    delete[] directories;

    // Figure out how many datablocks needed
    int nrOfBlocksNeeded = filesize / Block::BLOCK_SIZE;
    if((nrOfBlocksNeeded*Block::BLOCK_SIZE) < filesize || nrOfBlocksNeeded == 0)
        nrOfBlocksNeeded++;

    int freeInode = this->_findNextFreeInode();
    int* freeData = new int[nrOfBlocksNeeded];

    bool exist = true;
    for(int i = 0; i < nrOfBlocksNeeded; i++) {
        freeData[i] = this->_findNextFreeData();
        if(freeData[i] == -1)
            exist = false;
    }

    if(freeInode == -1 || !exist)
        throw "Error: no free spaces on harddrive.";

    int* toInodesIndexes;
    std::string* toDirectories;
    int toNrOfEntries = this->_getAllDirectoriesFromDataBlock(toInode, toInodesIndexes, toDirectories);
    int toInodeIndex = this->_findInodeIndexByName(toDirectories, toNrOfEntries, toFilename);

    delete[] toInodesIndexes;
    delete[] toDirectories;

    if (toInodeIndex != -1)
        throw "Error: filename exists.";

    // No errors - set bitmap to taken
    this->bitmapINodes[freeInode] = true;

    for(int i = 0; i < nrOfBlocksNeeded; i++)
        this->bitmapData[freeData[i]] = true;

    // Copy inode
    this->inodes[freeInode] = new INode(*fromInode);

    char cToFilename[15];
    for(int i = 0; i < 15; i++) {
        cToFilename[i] = toFilename[i];
    }
    this->inodes[freeInode]->setFilename(cToFilename);

    for (int i = 0; i < nrOfBlocksNeeded; i++)
        this->inodes[freeInode]->setSpecificDataBlock(i, freeData[i]);

    // Copy all dataBlocks
    int test = fromInode->getFirstDataBlockIndex();
    std::string dataBlock = this->_openData(test);

    char cDataBlock[Block::BLOCK_SIZE];
    for (int i = 0; i < Block::BLOCK_SIZE; i++) {
        cDataBlock[i] = dataBlock[i];
    }
    this->_writeData(this->inodes[freeInode]->getFirstDataBlockIndex(), cDataBlock);

    int iDataBlockFrom = fromInode->getNextDataBlockIndex();
    while(iDataBlockFrom != -1) {
        dataBlock = this->_openData(iDataBlockFrom);
        char cDataBlock[Block::BLOCK_SIZE];
        for (int i = 0; i < Block::BLOCK_SIZE; i++) {
            cDataBlock[i] = dataBlock[i];
        }
        this->_writeData(this->inodes[freeInode]->getNextDataBlockIndex(), cDataBlock);
        iDataBlockFrom = fromInode->getNextDataBlockIndex();
    }

    // Add inodeEntry to parent
    char cFilename[16] = {'\0'};
    cFilename[0] = (char) freeInode;
    for (int i = 0; i < 15; i++)
        cFilename[i + 1] = toFilename[i];

    std::string toDataBlock = this->_openData(toInode->getFirstDataBlockIndex());
    char cToDataBlock[Block::BLOCK_SIZE];
    for(int i = 0; i < Block::BLOCK_SIZE; i++)
        cToDataBlock[i] = toDataBlock[i];

    this->appendData(cToDataBlock, toInode->getFilesize(), cFilename, 16);
    this->_writeData(toInode->getFirstDataBlockIndex(), cToDataBlock);
    toInode->setFilesize(toInode->getFilesize()+16);

    delete[] freeData;
}

int FileSystem::appendFile(std::string to, std::string from) {
    std::string fromFilename;
    std::string toFilename;

    bool fromFound = _splitFilepath(fromFilename, from);
    bool toFound = _splitFilepath(toFilename, to);

    INode* fromInode;
    INode* toInode;

    int inodeIndexFrom;
    int inodeIndexTo;

    if(!fromFound) {
        fromInode = this->currentINode;
    } else {
        inodeIndexFrom = _goToFolder(from);
        fromInode = this->inodes[inodeIndexFrom];
    }

    if(!toFound) {
        toInode = this->currentINode;
    } else {
        inodeIndexTo = _goToFolder(to);
        toInode = this->inodes[inodeIndexTo];
    }

    int fromINodeIndex = this->_fileExists(fromInode, fromFilename);
    int toINodeIndex = this->_fileExists(toInode, toFilename);

    if (fromINodeIndex == -1 || toINodeIndex == -1) {
        throw "Error file does'nt exists.";
    }

    int *inodeIndexes;
    std::string *directories;
    int nrOfDirs = this->_getAllDirectoriesFromDataBlock(fromInode, inodeIndexes, directories);
    int inodeIndex = this->_findInodeIndexByName(directories, nrOfDirs, fromFilename);

    fromInode = this->inodes[inodeIndexes[inodeIndex]];

    bool* permissions;
    this->_checkPermissions(fromInode->getProtection(), permissions);

    if(!permissions[0])
        throw "Error: permission denied.";

    delete[] permissions;

    inodeIndexes = NULL;
    directories = NULL;
    nrOfDirs = this->_getAllDirectoriesFromDataBlock(toInode, inodeIndexes, directories);
    inodeIndex = this->_findInodeIndexByName(directories, nrOfDirs, toFilename);

    toInode = this->inodes[inodeIndexes[inodeIndex]];

    delete[] inodeIndexes;
    delete[] directories;
    this->_checkPermissions(toInode->getProtection(), permissions);

    if(!permissions[1])
        throw "Error: permission denied.";

    delete[] permissions;

    if (fromInode->isDir() || toInode->isDir())
        throw "Error: can only append to files.";

    unsigned int fromFileSize = fromInode->getFilesize();
    unsigned int toFileSize = toInode->getFilesize();

    if ((fromFileSize + toFileSize) > INode::MAX_FILESIZE)
        throw "Error: file's to big to fit inside.";

    int fromBlockIndex = fromInode->getFirstDataBlockIndex();
    std::string fromContent = this->_openData(fromBlockIndex);

    fromBlockIndex = fromInode->getNextDataBlockIndex();
    while (fromBlockIndex != -1) {
        fromContent += this->_openData(fromBlockIndex);
        fromBlockIndex = fromInode->getNextDataBlockIndex();
    }

    fromContent = fromContent.substr(0, (unsigned long)fromFileSize);

    int toLastBlockIndex = toInode->getFirstDataBlockIndex();
    int toLastTmpBlockIndex = toLastBlockIndex;
    toLastBlockIndex = toInode->getNextDataBlockIndex();

    int toBlockCounter = 1;
    while (toLastBlockIndex != -1) {
        toLastTmpBlockIndex = toLastBlockIndex;
        toLastBlockIndex = toInode->getNextDataBlockIndex();
        toBlockCounter++;
    }

    int tmpToFileSize = toFileSize;
    for (int i = 0; i < toBlockCounter - 1; i++)
        tmpToFileSize -= Block::BLOCK_SIZE;

    int byteRemainingTo = Block::BLOCK_SIZE - tmpToFileSize;
    int tmpFromFileSize = fromFileSize - byteRemainingTo;

    int nrOfBlocksNeeded = tmpFromFileSize / Block::BLOCK_SIZE;
    if (nrOfBlocksNeeded * Block::BLOCK_SIZE < tmpFromFileSize)
        nrOfBlocksNeeded++;

    int *freeDataBlocks = new int[nrOfBlocksNeeded];
    bool freeFound = true;
    for (int i = 0; i < nrOfBlocksNeeded; i++) {
        freeDataBlocks[i] = this->_findNextFreeData();
        if (freeDataBlocks[i] == -1)
            freeFound = false;
    }

    if (!freeFound)
        throw "Error: no free data blocks.";

    char cToLastBlockData[Block::BLOCK_SIZE];
    std::string toLastBlockData = this->_openData(toLastTmpBlockIndex);
    for (int i = 0; i < Block::BLOCK_SIZE; i++)
        cToLastBlockData[i] = toLastBlockData[i];

    for (int i = tmpToFileSize; i < Block::BLOCK_SIZE; i++)
        cToLastBlockData[i] = fromContent[i - tmpToFileSize];

    this->_writeData(toLastTmpBlockIndex, cToLastBlockData);

    int tmpBlockIndex = toBlockCounter;
    for (int i = 0; i < nrOfBlocksNeeded; i++)
        toInode->setSpecificDataBlock(tmpBlockIndex++, freeDataBlocks[i]);

    int newBlockIndex = toLastTmpBlockIndex + 1;
    int startFrom = byteRemainingTo;
    while (newBlockIndex != -1) {
        char cDataBlock[Block::BLOCK_SIZE];
        for (int i = 0; i < Block::BLOCK_SIZE; i++) {
            cDataBlock[i] = fromContent[startFrom + i];
        }
        this->_writeData(newBlockIndex, cDataBlock);

        startFrom += Block::BLOCK_SIZE;
        newBlockIndex = toInode->getNextDataBlockIndex();
    }

    toInode->setFilesize(toInode->getFilesize() + fromInode->getFilesize());

    delete[] freeDataBlocks;

    return 0;
}

void FileSystem::changePermission(std::string permission, std::string filepath) {
    int iPermission = std::stoi(permission, NULL);
    if(iPermission < 1 || iPermission > 7)
        throw "Error: invalid permission";

    std::string filename;
    bool found = this->_splitFilepath(filename, filepath);

    INode* location;
    if(!found) {
        location = this->currentINode;
    } else {
        int inodeIndex = this->_goToFolder(filepath);
        if(inodeIndex == -1)
            throw "Error: invalid filepath.";
        location = this->inodes[inodeIndex];
    }

    int* inodeIndexes;
    std::string* directories;
    int nrOfDirs = this->_getAllDirectoriesFromDataBlock(location, inodeIndexes, directories);
    int inodeIndex = this->_findInodeIndexByName(directories, nrOfDirs, filename);

    if (inodeIndex == -1) {
        delete[] inodeIndexes;
        delete[] directories;
        throw "Error: invalid filepath.";
    }

    location = this->inodes[inodeIndexes[inodeIndex]];

    delete[] inodeIndexes;
    delete[] directories;

    bool* permissions;
    this->_checkPermissions(location->getProtection(), permissions);

    if(!permissions[1]) {
        int counter = 0;
        bool passOK = false;
        do {
            std::cout << "[sudo] password: ";
            std::string password;
            std::getline(std::cin, password);

            if(password != "root") counter++;
            else passOK = true;
        } while (counter < 3 && !passOK);

        if(!passOK) {
            delete[] permissions;
            throw "Error: permission denied.";
        }
    }

    delete[] permissions;
    location->setProtection((const unsigned) iPermission);
}

void FileSystem::saveFilesystem(std::string path) {
    std::ofstream outFile;
    outFile.open(path, std::ios::out | std::ios::binary);

    if(!outFile.is_open()) {
        throw "Error: could not open filestream.";
    }

    // Writing all bitmaps to inode and datablocks
    for(int i = 0; i < 250; i++) {
        outFile.write((char *) &this->bitmapINodes[i], sizeof(bool));
        outFile.write((char *) &this->bitmapData[i], sizeof(bool));
    }

    // Writing inodes
    for(int i = 0; i < 250; i++) {
        if(this->bitmapINodes[i]) { // Finding used inode from bitmap
            outFile.write((char *) this->inodes[i], sizeof(INode));

            //Write dataBlock index from inode
            int nrOfBlockIndex = this->inodes[i]->getNrOfBlockIndex();
            for(int n = 0; n < nrOfBlockIndex; n++) {
                int blockIndex = this->inodes[i]->getSpecifikBlockIndex(n);
                outFile.write((char *) &blockIndex, sizeof(int));
            }
        }
    }

    // Writing datablocks
    for(int i = 0; i < 250; i++) {
        if(this->bitmapData[i]) { // Finding used datablock
            Block block = mMemblockDevice.readBlock(i);
            outFile.write((char *) &block, sizeof(Block));
        }
    }

    outFile.close();
}

void FileSystem::restoreFilesystem(std::string path) {
    std::ifstream inFile;
    inFile.open(path, std::ios::in | std::ios::binary);

    if(!inFile.is_open()) {
        throw "Error: no save-file found.";
    }

    // Resetting filesystem
    this->_reset();

    // Reading all bitmaps to inodes and datablock
    for(int i = 0; i < 250; i++) {
        inFile.read((char *) &this->bitmapINodes[i], sizeof(bool));
        inFile.read((char *) &this->bitmapData[i], sizeof(bool));
    }

    //Reading inodes
    for(int i = 0; i < 250; i++) {
        if(this->bitmapINodes[i]){ // Finding used inode from bitmap
            INode inode;
            inFile.read((char*)&inode, sizeof(INode));
            this->inodes[i] = new INode(inode);

            // Reading dataBlock index to inode
            int blockIndexes = this->inodes[i]->getNrOfBlockIndex();
            for (int n = 0; n < blockIndexes; n++) {
                int blockIndex;
                inFile.read((char*)&blockIndex, sizeof(int));
                this->inodes[i]->setSpecificDataBlock(n, blockIndex);
            }
        }
    }

    // Reading dataBlocks
    for(int i = 0; i < 250; i++) {
        if(this->bitmapData[i]) { // Finding used dataBlock from bitmap
            Block block;
            inFile.read((char *) &block, sizeof(Block));
            this->mMemblockDevice.writeBlock(i, block);
        }
    }

    this->currentINode = this->inodes[0];
}