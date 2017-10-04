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

    std::string openData(int blockIndex);
    int _appendData (char* dataBlock, int currentBlockSize, const char* data, int dataSize);

public:
    FileSystem();
    ~FileSystem();

    void format();

    /* These API functions need to be implemented
	   You are free to specify parameter lists and return values
    */
    /* This function creates a file in the filesystem */
    // createFile(...)

    /* Creates a folder in the filesystem */
    // createFolderi(...);

    /* Removes a file in the filesystem */
    // removeFile(...);

    /* Removes a folder in the filesystem */
    // removeFolder(...);

    /* Function will move the current location to a specified location in the filesystem */
    // goToFolder(...);

    /* This function will get all the files and folders in the specified folder */
    // listDir(...);

    /* Add your own member-functions if needed */
    std::string getPWD() const;
    int createInode(unsigned short int parentInodeIndex, std::string filename, unsigned short int protection, std::string creator, std::string owner, std::string pwd, unsigned short int filesize, bool isDir, bool isHidden = false);

    int appendData (char* dataBlock, int currentBlockSize, const char* data, int dataSize);
    int writeData(int dataBlock, char* data);

    int listDir (int* &inodes, std::string* &directories);

    //int openData()
    //int readData()
    //bool freeInode()
    //bool freeData()
};

#endif // FILESYSTEM_H
