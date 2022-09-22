//
// Created by gian on 15/09/22.
//
#ifndef DISS_SIMPLEPROTOTYPE_UTILITIES_CPP
#define DISS_SIMPLEPROTOTYPE_UTILITIES_CPP


#include "utilities.hpp"
#include <fstream>
#include <cerrno>
#include <cstring>
#include <sstream>


#endif //DISS_SIMPLEPROTOTYPE_UTILITIES_CPP

std::string showChar(const Byte &byte) {
    auto isPrintable = [](const Byte &c) {
        return (c >= ' ' && c <= '~');
    };

    auto charToString = [](const Byte &c) {
        return std::string(1, c);
    };

    auto unprintableToString = [&](const Byte &c) {
        switch (c) {
            case '\n':
                return "\\n";
            case '\t':
                return "\\t";
            default :
                return "[?]";
        }
    };

    if (isPrintable(byte)) return charToString(byte);
    else return unprintableToString(byte);
}

void printNewLine() {
    std::cout<<"\n";
}

size_t floor_log2(const size_t input) {  //this can definetly be optimised
    size_t pos = sizeof(decltype(input))*8-1;
    for (size_t tester = 1ULL<<pos; tester != 0; tester>>=1) {
        if (tester & input) return pos;
        pos--;
    }
}
std::string getErrorMessage() {
    //uses errno
    return std::strerror(errno);
}

template <class T>
bool getBitAt(const T item, const size_t pos) {  //pos is 0 indexed from the right
    return (item>>pos)&1;
}

template <class T>
std::string toBinaryString(const T item) {
    const size_t amountOfBits = bitsInType<T>();
    std::stringstream ss;
    for (int i=amountOfBits-1;i>=0;i--)
        ss << getBitAt(item, i);
    return ss.str();
}

void dumpFile(const std::string& fileName) {
    std::ifstream inStream(fileName, std::ios_base::binary);
    if (!inStream) {
        LOG("There was an error opening the file (for reading)! ", getErrorMessage());
    }

    std::size_t unitCount = 0;
    std::size_t unitsPerLine = 10;

    size_t inCurrentLine = 0;
    while (inStream) {
        Byte tempByte = inStream.get();
        if (inStream.eof()) {
            LOG("\nRead", unitCount, "units");
            break;
        }
        LOG_NONEWLINE_NOSPACES("[",toBinaryString(tempByte), "]");
        if (inCurrentLine >= unitsPerLine-1) {
            LOG("");
            inCurrentLine = 0;
        }
        else
            inCurrentLine++;
        unitCount++;
    }
}




size_t getFileSize(const std::string &fileName) {
    std::ifstream in(fileName, std::ifstream::ate | std::ifstream::binary);
    return in.tellg();
}
