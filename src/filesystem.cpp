#include "filesystem.h"
#include "INode.h"

FileSystem::FileSystem() {
	//this->reset();
	for (int i = 0; i < 25; i++) {
		this->inodes[i] = NULL;
	}
	this->inodes[0] = new INode("/", 7, "root", "root", "/", 0, true);
	this->currentINode = this->inodes[0];
}

FileSystem::~FileSystem() {
	this->reset();
}

void FileSystem::resetINodes() {
	for (int i = 0; i < 25; i++) {
		if (this->inodes[i] != NULL) {
			delete this->inodes[i];
			this->inodes[i] = NULL;
		}
	}
	this->currentINode = NULL;
}

void FileSystem::resetBitmapINodes() {
	for (int i = 0; i < 25; i++) {
		this->bitmapINodes[i] = false;
	}
}

void FileSystem::resetBitmapData() {
	for (int i = 0; i < 250; i++) {
		this->bitmapData[i] = false;
	}
}

void FileSystem::reset() {
	this->resetINodes();
	this->resetBitmapINodes();
	this->resetBitmapData();
}

std::string FileSystem::getPWD() const {
	return this->currentINode->getPWD();
}

/* Please insert your code */