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
    int findInodeByName(int* inodeIndexes, std::string* filenames, int size, std::string searchFilename);

    int getAllDirectoriesFromDataBlock (INode* inode, int* &inodes, std::string* &directories);

    std::string openData(int blockIndex);
    int _appendData (char* dataBlock, int currentBlockSize, const char* data, int dataSize);

    int goToFolder(std::string filePath);
public:
    FileSystem();
    ~FileSystem();

    /* These API functions need to be implemented
	   You are free to specify parameter lists and return values
    */

    /* This function creates a file in the filesystem */
    int createFile(std::string filepath, std::string username, const bool isDir);

    /* Creates a folder in the filesystem */
    //int createFolder(std::string filepath, std::string username);

    int createInode(unsigned short int parentInodeIndex, std::string filename, unsigned short int protection,
                        std::string creator, std::string owner, std::string pwd, bool isDir, bool isHidden);

    /* Removes a file in the filesystem */
    // removeFile(...);

    /* Removes a folder in the filesystem */
    // removeFolder(...);

    /* Function will move the current location to a specified location in the filesystem */

    int moveToFolder(const std::string filepath);

    /* This function will get all the files and folders in the specified folder */
    int listDir (int* &inodes, std::string* &directories);

    /* Add your own member-functions if needed */
    std::string getPWD() const;
    void format();

    int appendData (char* dataBlock, int currentBlockSize, const char* data, int dataSize);
    int writeData(int dataBlock, char *data);

    //int readData()
    //bool freeInode()
    //bool freeData()

    std::string cat(const std::string filepath);
};

#endif // FILESYSTEM_H
