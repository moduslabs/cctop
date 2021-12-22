//
// Created by Michael Schwartz on 12/15/21.
//

#include "../cctop.h"
#include "Debug.h"
#include <cstdarg>
#include <sys/stat.h>

Debug::Debug() {
#ifdef VERBOSE
    logfile = fopen(DEBUG_LOGFILE, "w");
    chmod(DEBUG_LOGFILE, 0666);
    fprintf(logfile, "\nBegin DEBUG log:\n");
    fflush(logfile);
#endif
}

Debug::~Debug() {
#ifdef VERBOSE
    fclose(logfile);
    logfile = nullptr;
#endif
}

void Debug::log(const char *fmt, ...) {
#ifdef VERBOSE
    va_list ap;

    char buffer[1024];
    va_start(ap, fmt);
    vsprintf(buffer, fmt, ap);
    va_end(ap);

    if (logfile != nullptr) {
        fprintf(logfile, "%s", buffer);
        fflush(logfile);
    }
#endif
}

void Debug::printw(const wchar_t *fmt, ...) {
#ifdef VERBOSE
    va_list ap;

    wchar_t buffer[1024];
    va_start(ap, fmt);
    vswprintf(buffer, 1024, fmt, ap);
    va_end(ap);

    if (logfile != nullptr) {
        fwprintf(logfile, L"%ls", buffer);
        fflush(logfile);
    }
#endif
}

Debug debug;