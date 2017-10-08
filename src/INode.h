#ifndef INODE_H
#define INODE_H

#include <string>

class INode {
public:
	static const unsigned short int MAX_FILESIZE = 5120;
	static const unsigned short int MAX_BLOCKSIZE = 512;
private:
	unsigned int protection;
	/*std::string filename;
	std::string creator;
	std::string owner;
	std::string pwd;*/

	char filename[15];
	char owner[10];
	char creator[10];
	char pwd[512];

	unsigned int filesize;
    unsigned int parentInodeIndex;
    unsigned int thisInodeIndex;
	int nrOfBlockIndex;
	bool _isHidden;
	bool _isDir;

	int data[10];
	unsigned int blockIndex;
public:

	INode();
	INode(unsigned short int parentInodeIndex, unsigned short int thisInodeIndex, char filename[], unsigned short int protection, char creator[], char owner[], char pwd[], unsigned short int filesize, bool isDir, bool isHidden = false);
	virtual ~INode();

	INode (const INode &other);

	std::string getFilename() const;
	void setFilename(char filename[]);

	unsigned int getProtection() const;
	void setProtection(const unsigned int protection);

	std::string getCreator() const;

	std::string getOwner() const;
	void setOwner(char owner[]);

	std::string getPWD() const;

	unsigned int getFilesize() const;
	void setFilesize(const unsigned int filesize);

	bool isHidden() const;
	void setHidden(const bool isHidden);

	bool isDir() const;

	int getFirstDataBlockIndex();

	int getNextDataBlockIndex();

    bool setDataBlock (int blockIndex);
	void setSpecificDataBlock (int dataindex, int blockIndex);

    unsigned int getParentInodeIndex () const;
    void setParentInodeIndex (unsigned int parentInodeIndex);

    unsigned int getThisInodeIndex() const;

	std::string toString();

	INode& operator =(const INode &other);
	void addBlockIndex ();
	int getNrOfBlockIndex () const;
	int getSpecifikBlockIndex (int dataindex);
};

#endif