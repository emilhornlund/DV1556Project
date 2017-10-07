#include "INode.h"
#include <iostream>

INode::INode(unsigned short int parentInodeIndex, unsigned short int thisInodeIndex, std::string filename, unsigned short int protection, std::string creator, std::string owner, std::string pwd, unsigned short int filesize, bool isDir, bool isHidden) {
	this->parentInodeIndex  = parentInodeIndex;
    this->thisInodeIndex    = thisInodeIndex;
    this->filename          = filename;
	this->protection        = protection;
	this->creator           = creator;
	this->owner             = owner;
	this->pwd               = pwd;
	this->filesize          = filesize;
	this->_isDir            = isDir;
	this->_isHidden         = isHidden;

	for (int i = 0; i < 10; i++) {
		this->data[i] = NULL;
	}
}

INode::INode(const INode &other) {
	this->parentInodeIndex  = other.parentInodeIndex;
	this->thisInodeIndex    = other.thisInodeIndex;
	this->filename          = other.filename;
	this->protection        = other.protection;
	this->creator           = other.creator;
	this->owner             = other.owner;
	this->pwd               = other.pwd;
	this->filesize          = other.filesize;
	this->_isDir            = other._isDir;
	this->_isHidden         = other._isHidden;


	for (int i = 0; i < 10; i++) {
		this->data[i] = NULL;
	}
}

INode::~INode() {
	for (int i = 0; i < 10; i++) {
		if (this->data[i] != NULL) {
			delete this->data[i];
			this->data[i] = NULL;
		}
	}
}

std::string INode::getFilename() const {
	return this->filename;
}

void INode::setFilename(const std::string filename) {
	this->filename = filename;
}

unsigned short int INode::getProtection() const {
	return this->protection;
}

void INode::setProtection(const unsigned short int protection) {
	this->protection = protection;
}

std::string INode::getCreator() const {
	return this->creator;
}

std::string INode::getOwner() const {
	return this->owner;
}

void INode::setOwner(const std::string owner) {
	this->owner = owner;
}

std::string INode::getPWD() const {
	return this->pwd;
}

unsigned short int INode::getFilesize() const {
	return this->filesize;
}

void INode::setFilesize(const unsigned short int filesize) {
	this->filesize = filesize;
}

bool INode::isHidden() const {
	return this->_isHidden;
}

void INode::setHidden(const bool isHidden) {
	this->_isHidden = isHidden;
}

bool INode::isDir() const {
	return this->_isDir;
}

short int INode::getFirstDataBlockIndex() {
	unsigned short int *tmp = this->data[0];
	if (tmp == NULL) {
		return -1;
	}
	this->blockIndex = 0;
	return *tmp;
}

short int INode::getNextDataBlockIndex() {
	this->blockIndex++;
	unsigned short int *tmp = this->data[this->blockIndex];
	if (tmp == NULL) {
		return -1;
	}
	return *tmp;
}

bool INode::setDataBlock (int blockIndex) {
    bool found = false;
    for (int i = 0; i < 10 && !found; i++) {
        if(this->data[i] == NULL) {
            this->data[i] = new unsigned short int(blockIndex);
            found = true;
        }
    }

    return found;
}

void INode::setSpecificDataBlock (int dataIndex, int blockIndex) {
	this->data[dataIndex] = new unsigned short (blockIndex);
}

unsigned short int INode::getParentInodeIndex () const {
    return this->parentInodeIndex;
}

void INode::setParentInodeIndex (unsigned short int parentInodeIndex) {
    if (parentInodeIndex < 0 || parentInodeIndex > 250)
        throw std::out_of_range("Exception: Inodeindex out of range");
    this->parentInodeIndex = parentInodeIndex;
}

unsigned short int INode::getThisInodeIndex() const {
    return this->thisInodeIndex;
}

/*
 * 1 X
 * 2 W
 * 3 WX
 * 4 R
 * 5 RX
 * 6 RW
 * 7 RWX
 * */
std::string INode::toString() {
	std::string retStr = "";
	int permission = this->protection;

	if ((permission - 4) >= 0) {
		retStr += "R";
		permission -= 4;
	} else {
		retStr += "-";
	}

	if ((permission - 2) >= 0) {
		retStr += "W";
		permission -= 2;
	} else {
		retStr += "-";
	}

	if ((permission - 1) >= 0) {
		retStr += "X ";
	} else {
		retStr += "- ";
	}

	retStr += "\t\t";

	if(!this->isDir())
		if(this->filesize < 100)
			retStr += std::to_string(this->filesize) + "\t\t";
		else
			retStr += std::to_string(this->filesize) + "\t";
	else
		retStr += "-\t\t";

	if(!this->isDir())
		retStr += "false\t";
	else
		retStr += "true\t";

	retStr += this->owner + "\t";
	retStr += this->creator + "\t";

	return retStr;
}

INode& INode::operator =(const INode &other) {
	this->parentInodeIndex  = other.parentInodeIndex;
	this->thisInodeIndex    = other.thisInodeIndex;
	this->filename          = other.filename;
	this->protection        = other.protection;
	this->creator           = other.creator;
	this->owner             = other.owner;
	this->pwd               = other.pwd;
	this->filesize          = other.filesize;
	this->_isDir            = other._isDir;
	this->_isHidden         = other._isHidden;


	for (int i = 0; i < 10; i++) {
		this->data[i] = NULL;
	}

	return *this;
}
