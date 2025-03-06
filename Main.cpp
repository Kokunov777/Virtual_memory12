#include <iostream>
#include <sstream>
#include "VirtualMemory.h"

int main() {
    setlocale(LC_ALL, "Russian");
    VirtualMemory* vm = nullptr;

    std::string command;
    std::cout << "Create/Input/Print/PrintBuffer/Exit" << std::endl;
    std::cout << "VM> ";

    while (std::getline(std::cin, command)) {
        std::stringstream ss(command);
        std::string action;
        ss >> action;

        if (action == "Create") {
            std::string filename, type;
            long size;
            int stringLength = 0;

            ss >> filename >> type;

            size_t openParenPos = type.find('(');

            if (openParenPos != std::string::npos) {
                std::string typeName = type.substr(0, openParenPos);
                std::string lengthStr = type.substr(openParenPos + 1, type.length() - openParenPos - 2);

                try {
                    stringLength = std::stoi(lengthStr);
                }
                catch (const std::invalid_argument& e) {
                    std::cerr << "Ошибка: Неверная длина строки." << std::endl;
                    std::cout << "VM> ";
                    continue;
                }

                type = typeName;
            }

            ss >> size;

            if (vm != nullptr) {
                delete vm;
            }

            try {
                vm = new VirtualMemory(filename, size, type, stringLength);
                std::cout << "Создана виртуальная память." << std::endl;
            }
            catch (const std::exception& e) {
                std::cerr << "Ошибка при создании виртуальной памяти: " << e.what() << std::endl;
            }
        }
        else if (action == "Input") {
            if (vm == nullptr) {
                std::cout << "Ошибка: сначала создайте виртуальную память." << std::endl;
            }
            else {
                long index;
                std::string value;

                ss >> index;
                std::getline(ss >> std::ws, value);

                if (value.length() > 1 && value.front() == '"' && value.back() == '"') {
                    value = value.substr(1, value.length() - 2);
                }

                if (vm->writeValue(index, value)) {
                    std::cout << "Записанное значение." << std::endl;
                }
                else {
                    std::cout << "Ошибка при записи значения." << std::endl;
                }
            }
        }
        else if (action == "Print") {
            if (vm == nullptr) {
                std::cout << "Ошибка: сначала создайте виртуальную память." << std::endl;
            }
            else {
                long index;
                ss >> index;
                std::string value;

                if (vm->readValue(index, value)) {
                    std::cout << "Значение по индексу " << index << ": " << value << std::endl;
                }
                else {
                    std::cout << "Значение считывания с ошибкой." << std::endl;
                }
            }
        }
        else if (action == "PrintBuffer") {
            if (vm != nullptr) {
                vm->printBufferContents();
            }
            else {
                std::cout << "Ошибка: сначала создайте виртуальную память." << std::endl;
            }
        }
        else if (action == "Exit") {
            break;
        }
        else {
            std::cout << "Недопустимая команда." << std::endl;
        }

        std::cout << "VM> ";
    }

    if (vm != nullptr) {
        delete vm;
    }

    return 0;
}