#include "VirtualMemory.h"
#include <iostream>
#include <cstring>
#include <sstream>
#include <algorithm>
#include <ctime>

const int PAGE_SIZE = 512;
const char SIGNATURE[2] = { 'V', 'M' };

int VirtualMemory::getPageIndex(long index) const {
    return index / (PAGE_SIZE / sizeof(char));
}

int VirtualMemory::getOffset(long index) const {
    return index % (PAGE_SIZE / sizeof(char));
}

Page* VirtualMemory::findPageInBuffer(int pageIndex) {
    for (auto& page : buffer) {
        if (page.absoluteNumber == pageIndex) {
            page.lastAccess = std::time(nullptr);
            return &page;
        }
    }
    return nullptr;
}

Page* VirtualMemory::getPage(int pageIndex) {
    Page* page = findPageInBuffer(pageIndex);
    if (page) return page;

    if (buffer.size() >= bufferCapacity) {
        evictPage();
    }

    buffer.emplace_back(PAGE_SIZE);
    Page& newPage = buffer.back();
    newPage.absoluteNumber = pageIndex;
    newPage.lastAccess = std::time(nullptr);

    file.seekg(2 + (long long)pageIndex * PAGE_SIZE, std::ios::beg);
    if (!file.read(newPage.data.data(), PAGE_SIZE)) {
        std::cerr << "Ошибка: Не удалось прочитать страницу из файла." << std::endl;
        buffer.pop_back();
        return nullptr;
    }

    return &newPage;
}

void VirtualMemory::evictPage() {
    if (buffer.empty()) return;

    auto oldestPage = std::min_element(buffer.begin(), buffer.end(), [](const Page& a, const Page& b) {
        return a.lastAccess < b.lastAccess;
        });

    if (oldestPage->modified) {
        file.seekp(2 + (long long)oldestPage->absoluteNumber * PAGE_SIZE, std::ios::beg);
        file.write(oldestPage->data.data(), PAGE_SIZE);
    }

    buffer.erase(oldestPage);
}

void VirtualMemory::flushBuffer() {
    for (auto& page : buffer) {
        if (page.modified) {
            file.seekp(2 + (long long)page.absoluteNumber * PAGE_SIZE, std::ios::beg);
            file.write(page.data.data(), PAGE_SIZE);
        }
    }
}

void VirtualMemory::createFile() {
    file.close();
    file.open(filename, std::ios::out | std::ios::binary | std::ios::trunc);
    if (!file.is_open()) {
        std::cerr << "Ошибка: Не удалось создать файл виртуальной памяти." << std::endl;
        return;
    }
    file.write(SIGNATURE, 2);
    for (int i = 0; i < numPages; ++i) {
        std::vector<char> emptyPage(PAGE_SIZE, 0);
        file.write(emptyPage.data(), PAGE_SIZE);
    }
    file.close();
    file.open(filename, std::ios::in | std::ios::out | std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Ошибка: Не удалось открыть файл виртуальной памяти для чтения/записи." << std::endl;
    }
}

VirtualMemory::VirtualMemory(const std::string& fname, long size, const std::string& type, int strLen)
    : filename(fname), arraySize(size), arrayType(type), stringLength(strLen) {
    numPages = (arraySize * sizeof(char) + PAGE_SIZE - 1) / PAGE_SIZE;

    file.open(filename, std::ios::in | std::ios::out | std::ios::binary);

    if (!file.is_open()) {
        createFile();
    }
    else {
        char signatureFromFile[2];
        file.read(signatureFromFile, 2);
        if (signatureFromFile[0] != SIGNATURE[0] || signatureFromFile[1] != SIGNATURE[1]) {
            std::cerr << "Ошибка: Неверная подпись в файле виртуальной памяти. Создаем новый файл." << std::endl;
            file.close();
            createFile();
        }
    }
}

VirtualMemory::~VirtualMemory() {
    flushBuffer();
    file.close();
}

bool VirtualMemory::writeValue(long index, const std::string& value) {
    if (index < 0 || index >= arraySize) {
        std::cerr << "Ошибка: Индекс выходит за пределы допустимых значений." << std::endl;
        return false;
    }

    int pageIndex = getPageIndex(index);
    int offset = getOffset(index);
    Page* page = getPage(pageIndex);

    if (!page) {
        std::cerr << "Ошибка: Не удалось восстановить страницу." << std::endl;
        return false;
    }

    if (arrayType == "int") {
        try {
            int intValue = std::stoi(value);
            std::memcpy(&page->data[offset], &intValue, sizeof(int));
        }
        catch (const std::invalid_argument& e) {
            std::cerr << "Ошибка: Недопустимое целочисленное значение: " << value << std::endl;
            return false;
        }
        catch (const std::out_of_range& e) {
            std::cerr << "Ошибка: Целочисленное значение выходит за пределы допустимого диапазона: " << value << std::endl;
            return false;
        }
    }
    else if (arrayType == "char") {
        if (value.length() > 0) {
            page->data[offset] = value[0];
        }
        else {
            page->data[offset] = '\0';
        }
    }
    else if (arrayType == "varchar") {
        size_t len = value.length();
        if (len > stringLength) {
            std::cerr << "Ошибка: Длина строки превышает предел varchar." << std::endl;
            return false;
        }
        std::memcpy(&page->data[offset], value.c_str(), len + 1);
    }

    page->modified = true;
    page->lastAccess = std::time(nullptr);
    page->bitmap[offset] = true;

    return true;
}

bool VirtualMemory::readValue(long index, std::string& value) {
    if (index < 0 || index >= arraySize) {
        std::cerr << "Ошибка: Индекс выходит за пределы допустимых значений." << std::endl;
        return false;
    }

    int pageIndex = getPageIndex(index);
    int offset = getOffset(index);

    Page* page = getPage(pageIndex);

    if (!page) {
        std::cerr << "Ошибка: Не удалось восстановить страницу." << std::endl;
        return false;
    }

    if (arrayType == "int") {
        int intValue;
        std::memcpy(&intValue, &page->data[offset], sizeof(int));
        value = std::to_string(intValue);
    }
    else if (arrayType == "char") {
        value = page->data[offset];
    }
    else if (arrayType == "varchar") {
        value = &page->data[offset];
    }

    return true;
}

std::string VirtualMemory::operator[](long index) {
    std::string value;
    if (readValue(index, value)) {
        return value;
    }
    else {
        return "";
    }
}

void VirtualMemory::printBufferContents() {
    std::cout << "Содержимое буфера:" << std::endl;
    for (const auto& page : buffer) {
        std::cout << "  Номер страницы: " << page.absoluteNumber << ", Модифицированный: " << page.modified
            << ", Последний доступ: " << page.lastAccess << std::endl;

        std::cout << "    Данные (первые 10 байт): ";
        for (int i = 0; i < std::min<size_t>(10, page.data.size()); ++i) {
            std::cout << static_cast<int>(page.data[i]) << " ";
        }
        std::cout << std::endl;
    }
}