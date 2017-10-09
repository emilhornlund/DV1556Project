#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include "memblockdevice.h"
#include "INode.h"

class FileSystem
{
private:
    MemBlockDevice  mMemblockDevice;
    INode           *inodes[MemBlockDevice::MEM_SIZE];
    INode           *currentINode;
    short int       bitmapInodeIndex;
    short int       bitmapDataIndex;
    bool            bitmapINodes[MemBlockDevice::MEM_SIZE];
    bool            bitmapData[MemBlockDevice::MEM_SIZE];

    void _resetINodes();
    void _resetBitmapINodes();
    void _resetBitmapData();
    void _reset();
    void _init();

    int         _findNextFreeInode();
    int         _findNextFreeData();
    int         _findInodeByName(int *inodeIndexes, std::string *filenames, int size, std::string searchFilename);
    int         _findInodeIndexByName(std::string *filenames, int size, std::string searchFilename);
    int         _getAllDirectoriesFromDataBlock(INode *inode, int *&inodes, std::string *&directories);
    std::string _openData(int blockIndex);
    int         _appendData (char* dataBlock, int currentBlockSize, const char* data, int dataSize);
    int         _goToFolder(std::string &filePath);
    int         _removeFolderEntry(INode *inode, std::string filename);
    bool        _splitFilepath(std::string &filename, std::string &path);
    int         _fileExists(INode *inode, std::string filename);
    void        _checkPermissions(int permission, bool *&permissions);
    void        _writeData(int dataBlock, char *data);

public:
    FileSystem();
    ~FileSystem();

    int         createFile(std::string filepath, std::string username, std::string &content, const bool isDir);
    int         createInode(unsigned int parentInodeIndex, std::string filename, unsigned int protection,
                        std::string creator, std::string owner, std::string pwd, bool isDir, bool isHidden);
    bool        removeFile(std::string filepath);
    int         moveToFolder();
    int         moveToFolder(std::string &filepath);
    std::string listDir ();
    std::string listDir (std::string &filepath);
    std::string getPWD() const;
    void        format();
    int         appendData (char* dataBlock, int currentBlockSize, const char* data, int dataSize);

    std::string cat(std::string &filepath);
    void        move (std::string from, std::string to);
    void        copy (std::string from, std::string to);
    int         appendFile(std::string to, std::string from);
    void        changePermission(std::string permission, std::string filepath);
    void        saveFilesystem(std::string path);
    void        restoreFilesystem(std::string path);
};

#endif // FILESYSTEM_H
