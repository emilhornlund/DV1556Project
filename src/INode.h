#ifndef INODE_H
#define INODE_H

#include <string>

class INode {
private:
	static const unsigned short int MAX_FILESIZE = 5120;
	static const unsigned short int MAX_BLOCKSIZE = 512;

	unsigned short int protection;
	std::string filename;
	std::string creator;
	std::string owner;
	std::string pwd;
	unsigned short int filesize;
    unsigned short int parentInodeIndex;
    unsigned short int thisInodeIndex;
	bool _isHidden;
	bool _isDir;

	unsigned short int *data[10];

	unsigned short int blockIndex;

public:

	INode(unsigned short int parentInodeIndex, unsigned short int thisInodeIndex, std::string filename, unsigned short int protection, std::string creator, std::string owner, std::string pwd, unsigned short int filesize, bool isDir, bool isHidden = false);
	virtual ~INode();

	INode (const INode &other);

	std::string getFilename() const;
	void setFilename(const std::string filename);

	unsigned short int getProtection() const;
	void setProtection(const unsigned short int protection);

	std::string getCreator() const;

	std::string getOwner() const;
	void setOwner(const std::string owner);

	std::string getPWD() const;

	unsigned short int getFilesize() const;
	void setFilesize(const unsigned short int filesize);

	bool isHidden() const;
	void setHidden(const bool isHidden);

	bool isDir() const;

	short int getFirstDataBlockIndex();

	short int getNextDataBlockIndex();

    bool setDataBlock (int blockIndex);
	void setSpecificDataBlock (int dataindex, int blockIndex);

    unsigned short int getParentInodeIndex () const;
    void setParentInodeIndex (unsigned short int parentInodeIndex);

    unsigned short int getThisInodeIndex() const;

	std::string toString();

	INode& operator =(const INode &other);
};

#endif