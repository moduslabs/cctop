#ifndef CCTOP_CCTOP_H
#define CCTOP_CCTOP_H

#define USE_NCURSES
//#undef USE_NCURSES

// define VERBOSE to enable debug printing
#define VERBOSE
//#undef VERBOSE
// debug print to this file:
#define DEBUG_LOGFILE "/tmp/cctop.log"

#ifdef USE_NCURSES
#define _XOPEN_SOURCE_EXTENDED
#include <ncurses.h>
#endif

#include "common/Debug.h"

#include "lib/Console.h"
#include "lib/Options.h"
#include "lib/Help.h"

#include "macos/Platform.h"
#include "macos/CPU.h"
#include "macos/Memory.h"
#include "macos/Disk.h"
#include "macos/Network.h"
#include "macos/ProcessList.h"
#include "macos/Battery.h"

const int MIN_WIDTH = 96, MIN_HEIGHT = 30;

#endif //CCTOP_CCTOP_H
