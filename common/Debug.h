//
// Created by Michael Schwartz on 12/15/21.
//

#ifndef CCTOP_DEBUG_H
#define CCTOP_DEBUG_H

#include <cstdio>

class Debug {
public:
    Debug();
    ~Debug();
public:
    void log(const char *fmt, ...);
    void printw(const wchar_t *fmt, ...);
protected:
    FILE *logfile{nullptr};

};

extern Debug debug;

#endif //CCTOP_DEBUG_H
