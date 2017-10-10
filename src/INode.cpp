#include "INode.h"
#include <iostream>

INode::INode() {
	this->parentInodeIndex  = 0;
	this->thisInodeIndex    = 0;
	this->nrOfBlockIndex	= 0;

	for(int i = 0; i < 15; i++)
		this->filename[i] = '\0';

	this->protection        = 7;

	for(int i = 0; i < 10; i++) {
		this->owner[i] = '\0';
		this->creator[i] = '\0';
	}

	for(int i = 0; i < 512; i++){
		this->pwd[i] = '\0';
	}

	this->filesize          = 0;
	this->_isDir            = false;
	this->_isHidden         = false;

	for (int i = 0; i < 10; i++) {
		this->data[i] = -1;
	}
}

INode::INode(unsigned short int parentInodeIndex, unsigned short int thisInodeIndex, char filename[], unsigned short int protection, char creator[], char owner[], char pwd[], unsigned short int filesize, bool isDir, bool isHidden) {
	this->parentInodeIndex  = parentInodeIndex;
    this->thisInodeIndex    = thisInodeIndex;
	this->nrOfBlockIndex	= 0;
    this->protection        = protection;
    this->filesize          = filesize;
    this->_isDir            = isDir;
    this->_isHidden         = isHidden;

	for(int i = 0; i < 15; i++)
		this->filename[i] = filename[i];

	for(int i = 0; i < 10; i++)
		this->owner[i] = owner[i];

    for(int i = 0; i < 10; i++)
		this->creator[i] = creator[i];

	for(int i = 0; i < 512; i++)
		this->pwd[i] = pwd[i];

	for (int i = 0; i < 10; i++)
		this->data[i] = -1;
}

INode::INode(const INode &other) {
	if(this != &other) {
		this->parentInodeIndex  = other.parentInodeIndex;
		this->thisInodeIndex    = other.thisInodeIndex;
		this->nrOfBlockIndex    = other.nrOfBlockIndex;
        this->filesize          = other.filesize;
        this->_isDir            = other._isDir;
        this->_isHidden         = other._isHidden;
        this->blockIndex        = other.blockIndex;
        this->protection        = other.protection;

		for (int i = 0; i < 15; i++)
			this->filename[i] = other.filename[i];

		for (int i = 0; i < 10; i++)
			this->owner[i] = other.owner[i];

        for (int i = 0; i < 10; i++)
            this->creator[i] = other.creator[i];

		for (int i = 0; i < 512; i++)
			this->pwd[i] = other.pwd[i];

		for (int i = 0; i < 10; i++)
			this->data[i] = other.data[i];
	}
}

INode::~INode() {

}

void INode::setFilename(char filename[]) {
	for(int i = 0; i < 15; i++) {
		this->filename[i] = filename[i];
	}
	//this->filename = filename;
}

unsigned int INode::getProtection() const {
	return this->protection;
}

void INode::setProtection(const unsigned int protection) {
	this->protection = protection;
}

std::string INode::getPWD() const {
	std::string retStr = "";
	for(int i = 0; i < 512 && this->pwd[i] != '\0'; i++) {
		retStr += this->pwd[i];
	}

	return retStr;
}

unsigned int INode::getFilesize() const {
	return this->filesize;
}

void INode::setFilesize(const unsigned long filesize) {
	this->filesize = filesize;
}

bool INode::isDir() const {
	return this->_isDir;
}

int INode::getFirstDataBlockIndex() {
	this->blockIndex = 0;
	return this->data[0];
}

int INode::getNextDataBlockIndex() {
	this->blockIndex++;
	return this->data[this->blockIndex];
}

bool INode::setDataBlock (int blockIndex) {
    bool found = false;
    for (int i = 0; i < 10 && !found; i++) {
        if(this->data[i] == -1) {
            this->data[i] = blockIndex;
            found = true;
        }
    }

    return found;
}

void INode::setSpecificDataBlock (int dataIndex, int blockIndex) {
	this->data[dataIndex] = blockIndex;
}

unsigned int INode::getThisInodeIndex() const {
    return this->thisInodeIndex;
}

std::string INode::toString() {
	std::string retStr = "";
	int permission = this->protection;

    /*
     * 1 X
     * 2 W
     * 3 WX
     * 4 R
     * 5 RX
     * 6 RW
     * 7 RWX
    */

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

	if(!this->isDir()) {
		retStr += std::to_string(this->filesize) + "\t";
	}
	else
		retStr += "-\t";

	if(!this->isDir())
		retStr += "false\t";
	else
		retStr += "true\t";

	retStr += this->owner;
	retStr += "\t";
	retStr += this->creator;
	retStr += "\t";

	return retStr.c_str();
}

INode& INode::operator =(const INode &other) {
	if(this != &other) {
		this->parentInodeIndex  = other.parentInodeIndex;
		this->thisInodeIndex    = other.thisInodeIndex;
		this->nrOfBlockIndex    = other.nrOfBlockIndex;
        this->protection        = other.protection;
        this->filesize          = other.filesize;
        this->_isDir            = other._isDir;
        this->_isHidden         = other._isHidden;
        this->blockIndex        = other.blockIndex;

		for (int i = 0; i < 15 && other.filename[i] != '\0'; i++)
			this->filename[i] = other.filename[i];

        for (int i = 0; i < 10 && other.owner[i] != '\0'; i++)
            this->owner[i] = other.owner[i];

        for (int i = 0; i < 10 && other.creator[i] != '\0'; i++)
            this->creator[i] = other.creator[i];

		for (int i = 0; i < 512 && other.pwd[i] != '\0'; i++)
			this->pwd[i] = other.pwd[i];

		for (int i = 0; i < 10; i++)
			this->data[i] = other.data[i];
	}

	return *this;
}

void INode::addBlockIndex () { this->nrOfBlockIndex++; }
