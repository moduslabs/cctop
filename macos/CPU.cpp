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
#include "../cctop.h"
#include <mach/mach_host.h>
#include <unistd.h>

//    2581 ▁
//    2582 ▂
//    2583 ▃
//    2584 ▄
//    2585 ▅
//    2586 ▆
//    2587 ▇
//    2588 █
struct dots {
    wchar_t ch;
    uint16_t r, g, b;
} dots[] = {
        {L'▁', 128, 255, 128}, // 0x2581
        {L'▂', 192, 255, 192}, // 0x2582
        {L'▃', 255, 255, 0}, // 0x2583
        {L'▄', 255, 255, 0}, // 0x2584

        {L'▅', 255, 125, 125}, // 0x2585
        {L'▆', 255, 100, 100}, // 0x2586
        {L'▇', 255, 50,  50}, // 0x2587
        {L'█', 255, 0,   0}, // 0x2588
};

static void renderColor(int ndx) {
#ifdef USE_NCURSES
    switch (ndx) {
        case -1:
//            debug.log("x ");
            console.mode_clear();
            break;
        case 0:
//            debug.log("y ");
            console.fg_yellow();
            break;
        case 1:
//            debug.log("c ");
            console.fg_cyan();
            break;
        case 2:
//            debug.log("cb ");
            console.mode_bold();
            console.fg_cyan();
            break;
        case 3:
//            debug.log("b ");
            console.fg_blue();
            break;
        case 4:
//            debug.log("bb ");
            console.mode_bold();
            console.fg_blue();
            break;
        case 5:
//            debug.log("m ");
            console.fg_magenta();
            break;
        case 6:
//            debug.log("mb ");
            console.mode_bold();
            console.fg_magenta();
            break;
        case 7:
//            debug.log("rb ");
            console.fg_red();
            console.mode_bold();
            break;
        default:
//            debug.log("x ");
            console.mode_clear();
            break;
    }
#else
    switch (ndx) {
        case 0:
            console.fg_yellow();
            break;
        case 1:
            console.fg_cyan();
            break;
        case 2:
            console.fg_cyan();
            break;
        case 3:
            console.mode_bold();
            console.fg_blue();
            break;
        case 4:
            console.fg_magenta();
            break;
        case 5:
            console.fg_magenta();
            console.mode_bold();
            break;
        case 6:
            console.fg_red();
            break;
        case 7:
            console.fg_red();
            console.mode_bold();
            break;
        default:
            break;
    }
#endif
}

static void renderDot(int level) {
#ifdef USE_NCURSES
    renderColor(level);
    if (level >= 0 && level <= 7) {
        console.wprintf(L"%lc", dots[level].ch);
    } else {
        // leve = -1 (uninitialized)
        console.print(" ");
    }
    console.mode_clear();
#else
    if (level >= 0 && level <= 7) {
        renderColor(level);
//        console.fg_rgb(dots[ndx].r, dots[ndx].g, dots[ndx].b);
        console.wprintf(L"%lc", dots[level].ch);
//        console.fg_rgb(255, 255, 255);
        console.mode_clear();
    } else {
        console.print(" ");
    }
#endif
}

CPUCore::CPUCore() {
    for (int &i: history) {
        i = -1;
    }
}

void CPUCore::diff(CPUCore *newer, CPUCore *older) {
    this->user = newer->user - older->user;
    this->nice = newer->nice - older->nice;
    this->system = newer->system - older->system;
    this->idle = newer->idle - older->idle;
}

void CPUCore::addHistory(int h) {
    for (int i=0; i<CPU_HISTORY_SIZE-1; i++) {
        history[i] = history[i+1];
    }
    history[CPU_HISTORY_SIZE-1] = h;
}

void CPUCore::print() {
    double total = 100.,
            _user = double(this->user),
            _system = double(this->system),
            _nice = double(this->nice),
            _use = _user + _system + _nice,
//            _idle = this->idle > 100 ? 100. : double(this->idle),
    _idle = total - _use;

    int ndx = -1;
    if (_use < 12.5) {
        ndx = 0;
    } else if (_use < 25.0) {
        ndx = 1;
    } else if (_use < 37.5) {
        ndx = 2;
    } else if (_use < 50.0) {
        ndx = 3;
    } else if (_use < 62.5) {
        ndx = 4;
    } else if (_use < 75.0) {
        ndx = 5;
    } else if (_use < 87.5) {
        ndx = 6;
    } else {
        ndx = 7;
    }

    addHistory(ndx);

    console.wprintf(L"  %-6s %6.1f%% %6.1f%% %6.1f%% %6.1f%% %6.1f%% ",
                    this->name,
                    _use,
                    _user,
                    _system,
                    _nice,
                    _idle);

    renderDot(ndx);
//    console.mode_clear();


    console.mode_bold();
    console.print(" [");
    console.mode_clear();

    double use = _use / 10 * 2,
            left = 20 - use;

    if (_use > 0 && use == 0) {
        _use = 1;
        left--;
    }

    renderColor(ndx);
    for (int cnt = 0; cnt < int(use); cnt++) {
        console.wprintf(L"%lc", 0x25a0);
    }

    for (int cnt = 0; cnt < left; cnt++) {
        console.print(" ");
    }

    console.mode_clear();
    console.mode_bold();
    console.print("] ");
    console.mode_clear();

//    debug.log("\n");
    for (int i : history) {
//        debug.log("%d", i);
        renderColor(i);
        renderDot(i);
        console.mode_clear();
    }
//    debug.log("\n");
    console.newline();
}

CPU::CPU() {
    num_cores = this->read(this->current);
    copy(this->last, this->current);
    copy(this->delta, this->current);
    this->update();
}

struct cpusample {
    uint64_t totalSystemTime;
    uint64_t totalUserTime;
    uint64_t totalIdleTime;
};

static void sample(struct cpusample *sample) {
    kern_return_t kr;
    mach_msg_type_number_t count;
    host_cpu_load_info_data_t r_load;

    uint64_t totalSystemTime = 0, totalUserTime = 0, totalIdleTime = 0;

    count = HOST_CPU_LOAD_INFO_COUNT;
    kr = host_statistics(mach_host_self(), HOST_CPU_LOAD_INFO, (int *) &r_load, &count);
    if (kr != KERN_SUCCESS) {
        printf("oops: %d\n", kr);
        return;
    }

    sample->totalSystemTime = r_load.cpu_ticks[CPU_STATE_SYSTEM];
    sample->totalUserTime = r_load.cpu_ticks[CPU_STATE_USER] + r_load.cpu_ticks[CPU_STATE_NICE];
    sample->totalIdleTime = r_load.cpu_ticks[CPU_STATE_IDLE];
}

//                                                      ⢠⣿⣿                              │CPU ■■■■■■■■■■  26% ⡀⡀⡀⡀⡀   0°C│ │
uint16_t CPU::read(std::map<std::string, CPUCore *> &m) {
    cpusample sam;
    sample(&sam);
    processor_cpu_load_info_t cpuLoad;
    mach_msg_type_number_t processorMsgCount;
    natural_t processorCount;

    total_ticks = 0;
    /* kern_return_t err = */ host_processor_info(mach_host_self(), PROCESSOR_CPU_LOAD_INFO, &processorCount,
                                                  (processor_info_array_t *) &cpuLoad, &processorMsgCount);
    CPUCore *total = m["CPU"];
    if (!total) {
        total = m["CPU"] = new CPUCore;
        total->name = strdup("CPU");
    }
    total->system = total->user = total->nice = total->idle = 0;
    for (natural_t i = 0; i < processorCount; i++) {
        unsigned int *cpu_ticks = &cpuLoad[i].cpu_ticks[0];
        char name[3 + 3 + 1];
        sprintf(name, "CPU%d", i);
        CPUCore *cpu = m[name];
        if (!cpu) {
            cpu = m[name] = new CPUCore;
            cpu->name = strdup(name);
        }
        cpu->system = cpu_ticks[CPU_STATE_SYSTEM];
        cpu->user = cpu_ticks[CPU_STATE_USER];
        cpu->nice = cpu_ticks[CPU_STATE_NICE];
        cpu->idle = cpu_ticks[CPU_STATE_IDLE];
        total_ticks += cpu->system + cpu->user + cpu->idle + cpu->nice;

        total->system += cpu->system;
        total->user += cpu->user;
        total->nice += cpu->nice;
        total->idle += cpu->idle;
    }
    return (uint16_t) processorCount;
}

void CPU::copy(std::map<std::string, CPUCore *> &dst,
               std::map<std::string, CPUCore *> &src) {
    for (const auto &kv: src) {
        CPUCore *o = (CPUCore *) kv.second;
        const char *name = o->name;

        CPUCore *cpu = dst[name];
        if (!cpu) {
            cpu = dst[name] = new CPUCore;
            cpu->name = strdup(name);
        }
        cpu->user = o->user;
        cpu->system = o->system;
        cpu->idle = o->idle;
        cpu->nice = o->nice;
    }
}

void CPU::update() {
    this->copy(this->last, this->current);
    this->read(this->current);
    for (const auto &kv: this->delta) {
        CPUCore *cpu = (CPUCore *) kv.second;
        cpu->diff(this->current[cpu->name], this->last[cpu->name]);
    }
    CPUCore *cpu = this->delta["CPU"];
    cpu->user /= this->num_cores;
    cpu->system /= this->num_cores;
    cpu->nice /= this->num_cores;
    cpu->idle /= this->num_cores;
}

uint16_t CPU::print(bool newline) {
    uint16_t count = 0;
    CPUCore *cpu;

    console.inverseln("  %-6s %7s %7s %7s %7s %7s    %-5.5s                 %s", "[C]PUS", "Use", "User",
                      "System", "Nice", "Idle", "Gauge", "History");
    count++;

    cpu = this->delta["CPU"];
    cpu->print();
    count++;
    if (!options.condenseCPU) {
        for (int i = 0; i < num_cores; i++) {
            char name[32];
            sprintf(name, "CPU%d", i);
            cpu = this->delta[name];
            cpu->print();
            count++;
        }
    }
    if (newline) {
        console.newline();
        count++;
    }
    return count;
}

CPU processor;

