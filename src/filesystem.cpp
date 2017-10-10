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


    INode* rootINode = this->_createINode(0, "/", 7, "root", "root", "/", true, false);


    int dataBlockIndex = this->_findNextFreeData();
    this->bitmapData[dataBlockIndex] = true;
    rootINode->setDataBlock(dataBlockIndex);

    char cDataBlock[Block::BLOCK_SIZE] = {'\0'};
    char dataEntry[16] = {'\0'};

    //.
    dataEntry[0] = (char)rootINode->getThisInodeIndex();
    dataEntry[1] = '.';
    appendData(cDataBlock, rootINode->getFilesize(), dataEntry, 16);
    rootINode->setFilesize(rootINode->getFilesize() + 16);

    //..
    dataEntry[0] = (char)rootINode->getThisInodeIndex();
    dataEntry[2] = '.';
    appendData(cDataBlock, rootINode->getFilesize(), dataEntry, 16);
    rootINode->setFilesize(rootINode->getFilesize() + 16);

    rootINode->addBlockIndex();
    this->_writeData(dataBlockIndex, cDataBlock);

    this->currentINode = rootINode;
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

int FileSystem::_findDataBlockByFileSize(const unsigned long fileSize, int *&freeData) {
    unsigned int nrOfBlocks = fileSize / Block::BLOCK_SIZE;
    if (nrOfBlocks*Block::BLOCK_SIZE < fileSize || nrOfBlocks == 0)
        nrOfBlocks++;

    if (nrOfBlocks > INode::DATA_ENTRIES)
        throw "Error: data to large";

    freeData = new int[nrOfBlocks];

    bool exist = true;
    for(unsigned int i = 0; i < nrOfBlocks; i++) {
        freeData[i] = this->_findNextFreeData();
        if(freeData[i] == -1)
            exist = false;
    }

    if(!exist) {
        delete[] freeData;
        throw "Error: no free spaces on harddrive.";
    }
    return nrOfBlocks;
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

INode * FileSystem::_findParentINode(std::string &filePath, std::string &fileName) {
    bool isRelativePath = this->_splitFilepath(fileName, filePath);
    if (fileName.length() > 15) {
        throw "Error: fileName too long";
    }
    INode* inode;
    if (filePath.empty()) {
        inode = this->currentINode;
    } else {
        int inodeIndex = _goToFolder(filePath);
        if (inodeIndex == -1) {
            throw "Error: invalid filePath";
        }
        inode = this->inodes[inodeIndex];
    }
    return inode;
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
        permissions[i] = false;

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

int FileSystem::_getAllDirectoriesFromDataBlock(INode *&inode, int *&inodes, std::string *&directories) {
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

    //OBS!!!!!! inode innehåller parent!!!

    //Filename innehåller filnamnet

    int retVal = -1;


    int *inodeIndexes;
    std::string *directories;
    int nrOfDirectories = this->_getAllDirectoriesFromDataBlock(inode, inodeIndexes, directories);
    int inodeIndex = _findInodeByName(inodeIndexes, directories, nrOfDirectories, filename);
    int entryIndex = _findInodeIndexByName(directories, nrOfDirectories, filename);

    if (inodeIndex != -1) {

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

            if (nrOfFilesInDir > 2) {
                okToDelete = false;
                retVal = -2;
            }
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
            retVal = inodeIndex;
        }
    }

    delete[] inodeIndexes;
    delete[] directories;

    return retVal;
}

bool FileSystem::_splitFilepath(std::string &filename, std::string &path) {
    bool found = false;
    int i = (int)(path.length() - 1);
    for (; i >= 0 && !found; i--) {
        if (path[i] == '/') {
            found = true;
        } else {
            std::string tmp;
            tmp += path[i];

            filename = tmp + filename;
            path.pop_back();
        }
    }

    if (path.size() > 1 && path[path.size() - 1] == '/') {
        path.pop_back();
    }

    bool isRelativePath = true;

    if (path[0] == '/')
        isRelativePath = false;



    /*
    int n = i;
    if(found)
        n = i+2;
    filename = path.substr((n), path.length());

    if (!filename.empty()) {
        if (found)
            path.erase(i + 1, path.length());
        else

    }
    */

    return isRelativePath;
}



/* PUBLIC FUNCTIONS */
std::string FileSystem::getPWD() const {
    return this->currentINode->getPWD();
}

void FileSystem::createFile(std::string filePath, std::string username, std::string &content, const bool isDir) {
    //int inodeIndex = -1;

    std::string fileName;
    INode* parentINode = this->_findParentINode(filePath, fileName);

    if (!parentINode->isDir()) throw "create: cannot create: Not a directory";

    bool* permissions;
    this->_checkPermissions(parentINode->getProtection(), permissions);

    if(!permissions[1])
        throw "Error: permission denied.";

    delete[] permissions;

    if (this->_fileExists(parentINode, fileName) != -1) {
        if (isDir) {
            throw "Error: folder exists.";
        } else {
            throw "Error: file exists.";
        }
    }

    if (!fileName.empty()) {
        std::string newPWD = parentINode->getPWD() + fileName;

        unsigned long fileSize = content.length();
        int* freeData = NULL;
        int nrOfBlocks = 0;

        if (isDir) {
            int dataIndex = this->_findNextFreeData();
            if (dataIndex == -1) {
                throw "Error: no free datablocks available";
            }
            newPWD += "/";
        } else {
            nrOfBlocks = this->_findDataBlockByFileSize(fileSize, freeData);
        }

        INode* newINode = this->_createINode(parentINode->getThisInodeIndex(), fileName, 7, username, username, newPWD, isDir, false);

        if (isDir) {
            int dataBlockIndex = this->_findNextFreeData();
            this->bitmapData[dataBlockIndex] = true;
            newINode->setDataBlock(dataBlockIndex);

            char cDataBlock[Block::BLOCK_SIZE] = {'\0'};
            char dataEntry[16] = {'\0'};

            //.
            dataEntry[0] = (char)newINode->getThisInodeIndex();
            dataEntry[1] = '.';
            appendData(cDataBlock, newINode->getFilesize(), dataEntry, 16);
            newINode->setFilesize(newINode->getFilesize() + 16);

            //..
            dataEntry[0] = (char)parentINode->getThisInodeIndex();
            dataEntry[2] = '.';
            appendData(cDataBlock, newINode->getFilesize(), dataEntry, 16);
            newINode->setFilesize(newINode->getFilesize() + 16);

            newINode->addBlockIndex();
            this->_writeData(dataBlockIndex, cDataBlock);
        } else {
            for(unsigned int i = 0; i < nrOfBlocks; i++)
                this->bitmapData[freeData[i]] = true;

            newINode->setFilesize(fileSize);

            for (unsigned int i = 0; i < nrOfBlocks; i++)
                newINode->setSpecificDataBlock(i, freeData[i]);

            delete[] freeData;

            if (fileSize <= Block::BLOCK_SIZE) {
                std::string dataStr;
                dataStr = this->_openData(newINode->getFirstDataBlockIndex());
                newINode->addBlockIndex();

                char dataBlock[Block::BLOCK_SIZE] = {'\0'};
                char data[Block::BLOCK_SIZE] = {'\0'};

                for (unsigned int i = 0; i < fileSize; i++)
                    data[i] = content[i];

                this->appendData(dataBlock, 0, data, fileSize);
                this->_writeData(newINode->getFirstDataBlockIndex(), dataBlock);
            } else {
                ///PART 1
                int blockIndex = newINode->getFirstDataBlockIndex();
                newINode->addBlockIndex();

                std::string dataStr;
                dataStr = this->_openData(blockIndex);

                char dataBlock[Block::BLOCK_SIZE] = {'\0'};
                char data[Block::BLOCK_SIZE] = {'\0'};

                for (int i = 0; i < Block::BLOCK_SIZE; i++)
                    data[i] = content[i];

                this->appendData(dataBlock, 0, data, Block::BLOCK_SIZE);
                this->_writeData(blockIndex, dataBlock);
                fileSize -= Block::BLOCK_SIZE;

                /// PART 2
                int counter = 1;
                while (fileSize > Block::BLOCK_SIZE) {
                    blockIndex = newINode->getNextDataBlockIndex();
                    newINode->addBlockIndex();
                    dataStr = this->_openData(blockIndex);

                    char dataBlock[Block::BLOCK_SIZE] = {'\0'};
                    char data[Block::BLOCK_SIZE] = {'\0'};

                    for (int i = 0; i < Block::BLOCK_SIZE; i++)
                        data[i] = content[(counter * Block::BLOCK_SIZE) + i];

                    this->appendData(dataBlock, 0, data, Block::BLOCK_SIZE);
                    this->_writeData(blockIndex, dataBlock);
                    fileSize -= Block::BLOCK_SIZE;

                    counter++;
                }

                /// PART 3
                if (fileSize > 0) {
                    blockIndex = newINode->getNextDataBlockIndex();
                    newINode->addBlockIndex();
                    dataStr = this->_openData(blockIndex);

                    char dataBlock[Block::BLOCK_SIZE] = {'\0'};
                    char data[Block::BLOCK_SIZE] = {'\0'};

                    for (unsigned int i = 0; i < fileSize; i++)
                        data[i] = content[(counter * Block::BLOCK_SIZE) + i];

                    this->appendData(dataBlock, 0, data, fileSize);
                    this->_writeData(blockIndex, dataBlock);
                }
            }
        }
    }
}

INode *& FileSystem::_createINode(unsigned int parentInodeIndex, std::string filename, unsigned int protection,
                                  std::string creator, std::string owner, std::string pwd, bool isDir, bool isHidden) {
    int inodeIndex = this->_findNextFreeInode();

    if (inodeIndex == -1) {
        throw "Error: no free inodes left";
    }

    if (filename.length() < 1 || filename.length() > 15) {
        throw "Error: filename too long";
    }

    if (owner.length() > 10 || creator.length() > 10) {
        throw "Error: username or creatorname too long.";
    }

    if (pwd.length() > Block::BLOCK_SIZE) {
        throw "Error: filepath is too long.";
    }

    //Place inode entry in parent directory


    if (this->currentINode != NULL) {
        INode *parent = this->inodes[parentInodeIndex];
        int tmpFileSize = parent->getFilesize();
        if (tmpFileSize + 16 <= Block::BLOCK_SIZE) {
            //std::string tmpDataBlock = this->mMemblockDevice.readBlock(parent->getFirstDataBlockIndex()).toString();

            std::string dataBlock = this->_openData(parent->getFirstDataBlockIndex());

            char data[16] = {'\0'};
            data[0] = (char)inodeIndex;

            unsigned int i = 0;
            for (; i < filename.length(); i++) {
                data[(i + 1)] = filename[i];
            }

            /*
            if(i < 15) {
                data[++i] = '\0';
            }
            */

            //char* tmpData = new char[tmpDataBlock.size()];
            //std::copy(tmpDataBlock.begin(), tmpDataBlock.end(), tmpData);

            char cDataBlock[Block::BLOCK_SIZE];
            for (int i = 0; i < Block::BLOCK_SIZE; i++) {
                cDataBlock[i] = dataBlock[i];
            }

            this->appendData(cDataBlock, tmpFileSize, data, 16);

            this->_writeData(parent->getFirstDataBlockIndex(), cDataBlock);
            parent->setFilesize(parent->getFilesize()+16);
        } else {
            throw "Error: folder space out of bounds";
        }
    }

    //Create INode


    /*
    int tmpFileSize = 0;

    this->bitmapData[dataIndex] = true;
    */

    char cFilename[15] = {'\0'};
    char cOwner[10] = {'\0'};
    char cCreator[10] = {'\0'};
    char cPwd[512] = {'\0'};

    for(unsigned int i = 0; i < filename.length(); i++)
        cFilename[i] = filename[i];

    for(unsigned int i = 0; i < owner.length(); i++)
        cOwner[i] = owner[i];

    for (unsigned int i = 0; i < creator.length(); i++)
        cCreator[i] = creator[i];

    for(unsigned int i = 0; i < pwd.length(); i++)
        cPwd[i] = pwd[i];

    this->inodes[inodeIndex] = new INode(parentInodeIndex, inodeIndex, cFilename, protection, cCreator, cOwner, cPwd, 0, isDir);
    this->bitmapINodes[inodeIndex] = true;
    return this->inodes[inodeIndex];
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

std::string FileSystem::listDir(std::string &filePath) {
    std::string retStr = "Permission\tSize\tFolder\tOwner\tCreator\tName\n";
    INode* location;

    if (filePath == "") {
        location = this->currentINode;
    } else {
        int inodeIndex = this->_goToFolder(filePath);
        if (inodeIndex == -1) {
            throw "No such file or directory";
        }
        location = this->inodes[inodeIndex];
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
        retStr += this->inodes[inodesIndexes[i]]->toString() + " ";

        if (this->inodes[inodesIndexes[i]]->isDir()) {
            retStr += "\033[1;34m";
            retStr += directories[i] + "/";
            retStr += "\033[0m";
        } else {
            retStr += directories[i];
        }

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
    if (inodeIndex == -1) {
        throw "No such file or directory";
    }
    this->currentINode = this->inodes[inodeIndex];
    return inodeIndex;
}

std::string FileSystem::cat(std::string &filePath) {
    std::string fileName;
    INode* parentINode = this->_findParentINode(filePath, fileName);

    int* inodesIndexes;
    std::string* directories;
    int nrOfDirectories = this->_getAllDirectoriesFromDataBlock(parentINode, inodesIndexes, directories);

    int inodeIndex = this->_findInodeIndexByName(directories, nrOfDirectories, fileName);
    if(inodeIndex == -1) {
        delete[] inodesIndexes;
        delete[] directories;
        throw "Error: no such file or directory.";
    }
    INode* final = this->inodes[inodesIndexes[inodeIndex]];

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

    std::string content;
    content.reserve((unsigned long)filesize);

    int blockIndex = final->getFirstDataBlockIndex();
    content += this->_openData(blockIndex);

    blockIndex = final->getNextDataBlockIndex();
    while (blockIndex != -1) {
        content += this->_openData(blockIndex);
        blockIndex = final->getNextDataBlockIndex();
    }

    for (int i = 0; i < (filesize - 1); i++) {
        if(content[i] == '\\' && content[i+1] == 'n') {
            content.replace(i, 2, "\n");
        }
    }

    return content.c_str();
}

bool FileSystem::removeFile(std::string filePath) {
    bool didSucceed = false;
    if (filePath.empty()) {
        throw "No such file or directory";
    } else {
        std::string fileName;
        INode* parentINode = this->_findParentINode(filePath, fileName);
        int deletedInode = this->_removeFolderEntry(parentINode, fileName);

        if (deletedInode >= 0) {
            this->bitmapINodes[deletedInode] = false;
            delete this->inodes[deletedInode];
            this->inodes[deletedInode] = NULL;
            didSucceed = true;
        }
        else if (deletedInode == -2) {
            throw "Directory not empty";
        }
        else {
            throw "No such file or directory";
        }
    }
    return didSucceed;
}

void FileSystem::move(std::string fromFilePath, std::string toFilePath) {
    std::string fromFileName;
    std::string toFileName;

    INode* fromInode = _findParentINode(fromFilePath, fromFileName);
    INode* toInode = _findParentINode(toFilePath, toFileName);

    int inodeIndex = this->_fileExists(toInode, toFileName);
    if (inodeIndex != -1)
        throw "File already exists";

    int* inodeIndexes;
    std::string* directories;
    int nrOfDirs = this->_getAllDirectoriesFromDataBlock(fromInode, inodeIndexes, directories);
    int indexInode = this->_findInodeIndexByName(directories, nrOfDirs, fromFileName);


    if(indexInode == -1) {
        delete[] inodeIndexes;
        delete[] directories;
        throw "No such file or directory";
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

    // Removed fromFilePath entry and rename
    int deletedInode = this->_removeFolderEntry(fromInode, fromFileName);
    if (deletedInode == -1) {
        throw "Invalid filepath";
    }
    if (deletedInode == -2) {
        throw "Directory not empty";
    }

    char cToFilename[15];
    for (int i = 0; i < 15; i++)
        cToFilename[i] = toFileName[i];
    this->inodes[deletedInode]->setFilename(cToFilename);

    // Add in other folder
    char data[16] = {'\0'};
    data[0] = (char) deletedInode;
    for(int i = 0; i < 15; i++)
        data[i+1] = toFileName[i];

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

void FileSystem::copy (std::string fromFilePath, std::string toFilePath) {
    std::string fromFileName;
    std::string toFileName;

    INode* fromInode = _findParentINode(fromFilePath, fromFileName);
    INode* toInode = _findParentINode(toFilePath, toFileName);

    int* inodeIndexes;
    std::string* directories;
    int nrOfEntries = this->_getAllDirectoriesFromDataBlock(fromInode, inodeIndexes, directories);
    int inodeIndex = this->_findInodeIndexByName(directories, nrOfEntries, fromFileName);

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
    int toInodeIndex = this->_findInodeIndexByName(toDirectories, toNrOfEntries, toFileName);

    delete[] toInodesIndexes;
    delete[] toDirectories;

    if (toInodeIndex != -1)
        throw "Error: filename exists.";

    // No errors - set bitmap toFilePath taken
    this->bitmapINodes[freeInode] = true;

    for(int i = 0; i < nrOfBlocksNeeded; i++)
        this->bitmapData[freeData[i]] = true;

    // Copy inode
    this->inodes[freeInode] = new INode(*fromInode);

    char cToFilename[15];
    for(int i = 0; i < 15; i++) {
        cToFilename[i] = toFileName[i];
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

    // Add inodeEntry toFilePath parent
    char cFilename[16] = {'\0'};
    cFilename[0] = (char) freeInode;
    for (int i = 0; i < 15; i++)
        cFilename[i + 1] = toFileName[i];

    std::string toDataBlock = this->_openData(toInode->getFirstDataBlockIndex());
    char cToDataBlock[Block::BLOCK_SIZE];
    for(int i = 0; i < Block::BLOCK_SIZE; i++)
        cToDataBlock[i] = toDataBlock[i];

    this->appendData(cToDataBlock, toInode->getFilesize(), cFilename, 16);
    this->_writeData(toInode->getFirstDataBlockIndex(), cToDataBlock);
    toInode->setFilesize(toInode->getFilesize()+16);

    delete[] freeData;
}

int FileSystem::appendFile(std::string toFilePath, std::string fromFilePath) {
    std::string fromFileName;
    std::string toFileName;

    INode* fromInode = _findParentINode(fromFilePath, fromFileName);
    INode* toInode = _findParentINode(toFilePath, toFileName);

    int fromINodeIndex = this->_fileExists(fromInode, fromFileName);
    int toINodeIndex = this->_fileExists(toInode, toFileName);

    if (fromINodeIndex == -1 || toINodeIndex == -1) {
        throw "Error file does'nt exists.";
    }

    int *inodeIndexes;
    std::string *directories;
    int nrOfDirs = this->_getAllDirectoriesFromDataBlock(fromInode, inodeIndexes, directories);
    int inodeIndex = this->_findInodeIndexByName(directories, nrOfDirs, fromFileName);

    fromInode = this->inodes[inodeIndexes[inodeIndex]];

    bool* permissions;
    this->_checkPermissions(fromInode->getProtection(), permissions);

    if(!permissions[0])
        throw "Error: permission denied.";

    delete[] permissions;

    inodeIndexes = NULL;
    directories = NULL;
    nrOfDirs = this->_getAllDirectoriesFromDataBlock(toInode, inodeIndexes, directories);
    inodeIndex = this->_findInodeIndexByName(directories, nrOfDirs, toFileName);

    toInode = this->inodes[inodeIndexes[inodeIndex]];

    delete[] inodeIndexes;
    delete[] directories;
    this->_checkPermissions(toInode->getProtection(), permissions);

    if(!permissions[1])
        throw "Error: permission denied.";

    delete[] permissions;

    if (fromInode->isDir() || toInode->isDir())
        throw "Error: can only append toFilePath files.";

    unsigned int fromFileSize = fromInode->getFilesize();
    unsigned int toFileSize = toInode->getFilesize();

    if ((fromFileSize + toFileSize) > INode::MAX_FILESIZE)
        throw "Error: filesize toFilePath big.";

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

    for (int i = 0; i < nrOfBlocksNeeded; i++) {
        this->bitmapData[freeDataBlocks[i]] = true;
    }

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
    INode* parentINode = this->_findParentINode(filepath, filename);

    int* inodeIndexes;
    std::string* directories;
    int nrOfDirs = this->_getAllDirectoriesFromDataBlock(parentINode, inodeIndexes, directories);
    int inodeIndex = this->_findInodeIndexByName(directories, nrOfDirs, filename);

    if (inodeIndex == -1) {
        delete[] inodeIndexes;
        delete[] directories;
        throw "Error: invalid filepath.";
    }

    parentINode = this->inodes[inodeIndexes[inodeIndex]];

    delete[] inodeIndexes;
    delete[] directories;

    bool* permissions;
    this->_checkPermissions(parentINode->getProtection(), permissions);

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
    parentINode->setProtection((const unsigned) iPermission);
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