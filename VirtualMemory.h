#ifndef VIRTUALMEMORY_H
#define VIRTUALMEMORY_H

#include <string>
#include <fstream>
#include <vector>
#include "Page.h"

class VirtualMemory {
private:
    std::string filename;
    std::fstream file;
    std::vector<Page> buffer;
    long arraySize;
    int numPages;
    int bufferCapacity = 3;
    std::string arrayType;
    int stringLength;

    int getPageIndex(long index) const;
    int getOffset(long index) const;
    Page* findPageInBuffer(int pageIndex);
    Page* getPage(int pageIndex);
    void evictPage();
    void flushBuffer();
    void createFile();

public:
    VirtualMemory(const std::string& fname, long size, const std::string& type, int strLen = 0);
    ~VirtualMemory();

    bool writeValue(long index, const std::string& value);
    bool readValue(long index, std::string& value);
    std::string operator[](long index);
    void printBufferContents();
};

#endif // VIRTUALMEMORY_H