#ifndef CCTOP_OPTIONS_H
#define CCTOP_OPTIONS_H

#include <cstdint>

class Options {
public:
    bool showHelp{false};

    bool condenseMain{false},
            condenseCPU{false},
            condenseCPU_state{false},
            condenseMemory{false},
            condenseVirtualMemory{false},
            condenseDisk{false},
            condenseNetwork{false},
            condenseProcesses{false};

    uint64_t read_timeout{1000}; // in milliseconds
    uint16_t min_rows{0};

public:
    void process(int c);

};

extern Options options;

#endif //CCTOP_OPTIONS_H
