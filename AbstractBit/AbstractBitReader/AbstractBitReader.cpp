//
// Created by gian on 01/01/23.
//

#include "AbstractBitReader.hpp"

namespace GC {

    /**
     * Reads a fixed amount of bits and returns them into a single unsigned integer value
     * @param amountOfBits the amounts of bits that will be read (<=32)
     * @return the bits will be put onto an integer like so: 0 0 0 0 0 a b c d etc...
     */
    size_t AbstractBitReader::readAmountOfBits(const size_t amountOfBits) {
        size_t result = 0;
        for (size_t i = 0; i < amountOfBits; i++) {
            result <<= 1;
            result |= readBit();
        }
        return result;
    }

    /**
     * Reads 8 bits and returns the byte obtained from them
     * @return a byte value composed of the bits read
     */
    Byte AbstractBitReader::readByte() {
        return readAmountOfBits(8);
    }

    /**
     * Reads bits and puts them into a vector
     * @param amount the amount of bits to be read (>= 0)
     * @return the bool vector, where [0] is the first bit read
     */
    std::vector<bool> AbstractBitReader::readVector(const size_t amount) {
        std::vector<bool> result;
        repeat(amount, [&]() {result.push_back(readBit());});
        return result;
    }

    /**
     * Interprets and consumes the source to read a unary value
     * @return the value encoded by the source
     */
    size_t AbstractBitReader::readUnary() {
        size_t result = 0;
        while (!readBit())
            result++;
        return result;
    }

    /**
     * Reads and consumes a value encoded in a format which is efficient for smaller values (Exponential Rice encoding)
     * @return the decoded value
     */
    size_t AbstractBitReader::readSmallAmount() {
        return readRiceEncoded();
    }

    /**
     * Reads and consumes a value in Exponential Rice encoding
     * @return the decoded value
     */
    size_t AbstractBitReader::readRiceEncoded() {
        size_t bitSizeRead = readUnary();
        size_t bitSize = (bitSizeRead+1)*2;
        auto getOffsetOfBitSize = [&](const size_t bs) {
            auto pow4 = [&](auto n) {return 1LL<<(n*2);};
            auto sumPow4 = [&](auto n) {return (pow4(n)-1)/3;};
            return sumPow4(bitSize/2)-1;   //this is effectively bitSizeRead..
        };
        size_t contents = readAmountOfBits(bitSize);
        return contents+getOffsetOfBitSize(bitSize);
    }
} // GC