#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include "memblockdevice.h"

class INode;

class FileSystem
{
private:
    MemBlockDevice mMemblockDevice;
    INode *inodes[25];
    bool bitmapINodes[25];
    bool bitmapData[250];

    INode *currentINode;

    void resetINodes();
    void resetBitmapINodes();
    void resetBitmapData();
public:
    FileSystem();
    ~FileSystem();

    void reset();


    /* These API functions need to be implemented
	   You are free to specify parameter lists and return values
    */

    std::string getPWD() const;

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
};

#endif // FILESYSTEM_H
