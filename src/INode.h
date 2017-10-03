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
	bool _isHidden;
	bool _isDir;

	unsigned short int *data[10];

	unsigned short int blockIndex;

public:
	INode(std::string filename, unsigned short int protection, std::string creator, std::string owner, std::string pwd, unsigned short int filesize, bool isDir, bool isHidden = false);
	~INode();

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
};

#endif