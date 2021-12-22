/*
 * cctop for MacOS
 *
 * Programmed by Mike Schwartz <mike@moduscreate.com>
 *
 * Command line tool that refreshes the terminal/console window each second,
 * showing uptime, load average, CPU usage/stats, Memory/Swap usage, Disk
 * Activity (per drive/device), Virtual Memory activity (paging/swapping), and
 * Network traffic (per interface).
 *
 * Run this on a busy macos and you can diagnose if:
 * 1) System is CPU bound
 * 2) System is RAM bound
 * 3) System is Disk bound
 * 4) System is Paging/Swapping heavily
 * 5) System is Network bound
 *
 * To exit, hit ^C.
 */
#ifndef C_CPU_H
#define C_CPU_H

#include "../cctop.h"
#include <map>
#include <string>

const int CPU_HISTORY_SIZE = 20;

struct CPUCore {
    CPUCore();

    int id{};
    const char *name{};
    uint64_t user{}, nice{}, system{}, idle{};
    int history[CPU_HISTORY_SIZE]{};

    void diff(CPUCore *newer, CPUCore *older);

    void print();
    void addHistory(int h);
};

class CPU {
public:
    std::map<std::string, CPUCore *> last, current, delta;
    uint64_t total_ticks{};
    int num_cores;

public:
    CPU();

public:
    // returns # of processors
    uint16_t read(std::map<std::string, CPUCore *> &m);

    static void copy(std::map<std::string, CPUCore *> &dst,
              std::map<std::string, CPUCore *> &src);

    void update();

    uint16_t print(bool newline);
};

extern CPU processor;

#endif // C_CPU_H
