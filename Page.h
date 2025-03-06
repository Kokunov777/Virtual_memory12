#pragma once
#ifndef PAGE_H
#define PAGE_H

#include <vector>
#include <ctime>

struct Page {
	int absoluteNumber;
	bool modified;
	time_t lastAccess;
	std::vector<bool> bitmap;
	std::vector<char> data;

	Page(int pageSize);
};


#endif // PAGE_H
