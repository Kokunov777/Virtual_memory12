#include "Page.h"

Page::Page(int pageSize)
    : absoluteNumber(-1), modified(false), lastAccess(0), bitmap(pageSize, false), data(pageSize, 0) {
}