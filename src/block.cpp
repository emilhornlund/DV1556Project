#include "block.h"
#include <stdexcept>


Block::Block() {
    this->reset();
}

Block::Block(const Block &other) {
    for (int i = 0; i < BLOCK_SIZE; ++i)
        this->block[i] = other.block[i];
}

Block::~Block() {

}

Block &Block::operator =(const Block &other) {
    for (int i = 0; i < BLOCK_SIZE; ++i)
        this->block[i] = other.block[i];
    return *this;
}

char Block::operator[](int index) const {
    if (index < 0 || index >= BLOCK_SIZE) {
        throw std::out_of_range("Illegal access\n");
    }
    else {
        return this->block[index];
    }
}

void Block::reset(char c) {
    for (int i = 0; i < BLOCK_SIZE; ++i)
        this->block[i] = c;
}

int Block::size() const {
    return BLOCK_SIZE;
}

Block Block::readBlock() const {
    return Block(*this);
}

int Block::writeBlock(const std::string &strBlock) {
    int output = -2;    // Assume out of range
    if (strBlock.size() == (unsigned long)BLOCK_SIZE) {
        for (int i = 0; i < BLOCK_SIZE; ++i) {
            this->block[i] = strBlock[i];
        }
        output = 1;
    }

    return output;
}

int Block::writeBlock(const std::vector<char> &vec) {
    int output = -2; // Assume not the same dimension
    if (vec.size() == (unsigned long)BLOCK_SIZE) {
        for (unsigned long int i = 0; i < vec.size(); ++i) {
           this->block[i] = vec[i];
        }
        output = 1;
    }
//    else {
//        throw std::out_of_range("vector and block not the same dimension");
//    }
    return output;
}

void Block::writeBlock(const char cArr[]) {
    for (int i = 0; i < BLOCK_SIZE; ++i) {
        this->block[i] = cArr[i];
    }
}

std::string Block::toString() const {
    std::string output;
    output.reserve(BLOCK_SIZE);
    for (int i = 0; i < 512; ++i)
        output += this->block[i];
    return output;
}





