#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include "memblockdevice.h"
#include "INode.h"

class FileSystem
{
private:
    static const short int MAX_INT = 250;
    MemBlockDevice mMemblockDevice;
    INode *inodes[MAX_INT];
    INode *currentINode;
    short int bitmapInodeIndex;
    short int bitmapDataIndex;
    bool bitmapINodes[MAX_INT];
    bool bitmapData[MAX_INT];

    void resetINodes();
    void resetBitmapINodes();
    void resetBitmapData();
    void reset();
    void init();

    int findNextFreeInode();
    int findNextFreeData();
    int findInodeByName(int* inodeIndexes, std::string* filenames, int size, std::string searchFilename);
    int findInodeIndexByName(std::string* filenames, int size, std::string searchFilename);
    int getAllDirectoriesFromDataBlock (INode* inode, int* &inodes, std::string* &directories);
    std::string openData(int blockIndex);
    int _appendData (char* dataBlock, int currentBlockSize, const char* data, int dataSize);
    int goToFolder(std::string filePath);
    int removeFolderEntry (INode* inode, std::string filename);
    bool splitFilepath (std::string &filename, std::string &path);
    int fileExists(INode *inode, std::string filename);
    void checkPermissions(int permission, bool* &permissions);

    void writeInode (INode inode, std::ofstream &outFile);

public:
    FileSystem();
    ~FileSystem();

    int createFile(std::string filepath, std::string username, std::string &content, const bool isDir);
    int createInode(unsigned short int parentInodeIndex, std::string filename, unsigned short int protection,
                        std::string creator, std::string owner, std::string pwd, bool isDir, bool isHidden);
    bool removeFile(std::string filepath);
    int moveToFolder();
    int moveToFolder(const std::string filepath);
    std::string listDir ();
    std::string listDir (std::string &filepath);
    std::string getPWD() const;
    void format();
    int appendData (char* dataBlock, int currentBlockSize, const char* data, int dataSize);
    int writeData(int dataBlock, char *data);
    std::string cat(std::string &filepath);
    int move (std::string from, std::string to);
    int copy (std::string from, std::string to);
    int appendFile(std::string to, std::string from);
    int changePermission(std::string permission, std::string filepath);
    void saveFilesystem();
    void restoreFilesystem();
};

#endif // FILESYSTEM_H
