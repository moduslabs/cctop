//
// Created by Michael Schwartz on 12/11/21.
//

#include "Options.h"
#include <cstdlib>

#include "Console.h"

void Options::process(int c) {
    switch (c) {
        case 3:
        case 'q':
        case 'Q':
            console.abort("QUIT\n");
            exit(0);
        case 12:
            console.clear();
            showHelp = false;
            break;
        case 'c':
        case 'C':
            condenseCPU_state = !condenseCPU_state;
            showHelp = false;
            break;
        case 'm':
        case 'M':
            condenseMemory = !condenseMemory;
            showHelp = false;
            break;
        case 'v':
        case 'V':
            condenseVirtualMemory = !condenseVirtualMemory;
            showHelp = false;
            break;
        case 'd':
        case 'D':
            condenseDisk = !condenseDisk;
            showHelp = false;
            break;
        case 'n':
        case 'N':
            condenseNetwork = !condenseNetwork;
            showHelp = false;
            break;
        case 'p':
        case 'P':
            condenseProcesses = !condenseProcesses;
            showHelp = false;
            break;
        case 'x':
        case 'X':
            condenseMain = !condenseMain;
            showHelp = false;
            break;
        case '?':
        case 'h':
            showHelp = !showHelp;
            break;
        default:
            return;
    }
    console.clear();
}

Options options;
