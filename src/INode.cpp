#include "INode.h"
#include <iostream>

INode::INode(std::string filename, unsigned short int protection, std::string creator, std::string owner, std::string pwd, unsigned short int filesize, bool isDir, bool isHidden) {
	this->filename = filename;
	this->protection = protection;
	this->creator = creator;
	this->owner = owner;
	this->pwd = pwd;
	this->filesize = filesize;
	this->_isDir = isDir;
	this->_isHidden = isHidden;

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