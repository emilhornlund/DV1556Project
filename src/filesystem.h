#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include "memblockdevice.h"

class INode;

class FileSystem
{
private:

    static const short int MAX_INT = 250;
    MemBlockDevice mMemblockDevice;


    INode *inodes[MAX_INT];

    short int bitmapInodeIndex;
    short int bitmapDataIndex;
    bool bitmapINodes[MAX_INT];
    bool bitmapData[MAX_INT];

    INode *currentINode;

    void resetINodes();
    void resetBitmapINodes();
    void resetBitmapData();
    void reset();
    void init();

    int findNextFreeInode();
    int findNextFreeData();
    int findInodeByName(std::string filename);
    int getAllDirectoriesFromDataBlock (INode* inode, int* &inodes, std::string* &directories);

    int _appendData (char* dataBlock, int currentBlockSize, const char* data, int dataSize);

public:
    FileSystem();
    ~FileSystem();

    void format();
    /* This function creates a file in the filesystem */
    // createFile(...)

    /* Creates a folder in the filesystem */
    // createFolder(...);
    int createInode(unsigned short int parentInodeIndex, std::string filename, unsigned short int protection, std::string creator, std::string owner, std::string pwd, unsigned short int filesize, bool isDir, bool isHidden = false);

    /* Removes a file in the filesystem */
    // removeFile(...);

    /* Removes a folder in the filesystem */
    // removeFolder(...);

    /* Function will move the current location to a specified location in the filesystem */
    // goToFolder(...);

    /* Add your own member-functions if needed */
    std::string getPWD() const;
    int appendData (char* dataBlock, int currentBlockSize, const char* data, int dataSize);
    int writeData (int dataBlock, char* data);
    std::string openData(int blockIndex);
    int listDir (int* &inodes, std::string* &directories);

    //int readData()
    //bool freeInode()
    //bool freeData()
};

#endif // FILESYSTEM_H
