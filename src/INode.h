#ifndef INODE_H
#define INODE_H

#include <string>
#include "block.h"

class INode {
public:
    static const unsigned int DATA_ENTRIES = 10;
    static const unsigned int MAX_FILESIZE = Block::BLOCK_SIZE * DATA_ENTRIES;

private:
	unsigned int protection;

	char filename[15];
	char owner[10];
	char creator[10];
	char pwd[512];

	unsigned int 	filesize;
    unsigned int 	parentInodeIndex;
    unsigned int 	thisInodeIndex;
	int 			nrOfBlockIndex;
	bool 			_isHidden;
	bool 			_isDir;

	int 			data[DATA_ENTRIES];
	unsigned int 	blockIndex;

public:
	INode();
	INode (const INode &other);
	INode(unsigned short int parentInodeIndex, unsigned short int thisInodeIndex, char filename[], unsigned short int protection, char creator[], char owner[], char pwd[], unsigned short int filesize, bool isDir, bool isHidden = false);
	virtual ~INode();

	void setFilename(char filename[]);
	void setProtection(const unsigned int protection);
	void setFilesize(const unsigned int filesize);
	bool setDataBlock (int blockIndex);
	void setSpecificDataBlock (int dataindex, int blockIndex);

	unsigned int 	getProtection() const;
	std::string 	getPWD() const;
	unsigned int 	getFilesize() const;
	int 			getFirstDataBlockIndex();
	int 			getNextDataBlockIndex();
	unsigned int 	getThisInodeIndex() const;
	int 			getNrOfBlockIndex () const;
	int 			getSpecifikBlockIndex (int dataindex);

	bool 			isDir() const;
	std::string 	toString();
	void 			addBlockIndex ();

	INode& operator =(const INode &other);

};

#endif