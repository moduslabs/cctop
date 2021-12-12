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
}

static void renderDot(int ndx) {
    if (ndx >= 0 && ndx <= 7) {
        renderColor(ndx);
//        console.fg_rgb(dots[ndx].r, dots[ndx].g, dots[ndx].b);
        console.wprint(L"%lc", dots[ndx].ch);
//        console.fg_rgb(255, 255, 255);
        console.mode_clear();
    } else {
        console.print(" ");
    }
}

CPU::CPU() {
    for (int &i: history) {
        i = -1;
    }
}

void CPU::diff(CPU *newer, CPU *older) {
    this->user = newer->user - older->user;
    this->nice = newer->nice - older->nice;
    this->system = newer->system - older->system;
    this->idle = newer->idle - older->idle;
}

void CPU::print() {
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
    for (int i = 0; i < CPU_HISTORY_SIZE - 1; i++) {
        history[i] = history[i + 1];
    }

    wchar_t c = dots[ndx].ch;
    history[CPU_HISTORY_SIZE - 1] = ndx;

    renderColor(ndx);
    console.wprint(L"  %-6s %6.1f%% %6.1f%% %6.1f%% %6.1f%% %6.1f%% ",
                   this->name,
                   _use,
                   _user,
                   _system,
                   _nice,
                   _idle);
    console.mode_clear();
    renderDot(ndx);
    console.print(" [");

    double use = _use / 10 * 2,
            left = 20 - use;
    if (_use > 0 && use == 0) {
        use = 1;
        left--;
    }
    for (int cnt = 0; cnt < int(use); cnt++) {
        if (cnt < _use / 2) {
            console.fg_yellow();
        } else {
            console.fg_red();
        }
        console.print(">");
        console.reset();
    }
    for (int cnt = 0; cnt < left; cnt++) {
        console.print(" ");
    }
    console.print("] ");

    for (wchar_t i: history) {
        renderDot(i);
//        console.wprint(L"%lc", i);
    }
    console.newline();
//    if (total != 0) {
//        console.wprintln(L" %lc", c);
//    }
}

Processor::Processor() {
    this->num_cores = this->read(this->current);
    this->copy(this->last, this->current);
    this->copy(this->delta, this->current);
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
uint16_t Processor::read(std::map<std::string, CPU *> &m) {
    cpusample sam;
    sample(&sam);
    processor_cpu_load_info_t cpuLoad;
    mach_msg_type_number_t processorMsgCount;
    natural_t processorCount;

    total_ticks = 0;
    /* kern_return_t err = */ host_processor_info(mach_host_self(), PROCESSOR_CPU_LOAD_INFO, &processorCount,
                                                  (processor_info_array_t *) &cpuLoad, &processorMsgCount);
    CPU *total = m["CPU"];
    if (!total) {
        total = m["CPU"] = new CPU;
        total->name = strdup("CPU");
    }
    total->system = total->user = total->nice = total->idle = 0;
    for (natural_t i = 0; i < processorCount; i++) {
        unsigned int *cpu_ticks = &cpuLoad[i].cpu_ticks[0];
        char name[3 + 3 + 1];
        sprintf(name, "CPU%d", i);
        CPU *cpu = m[name];
        if (!cpu) {
            cpu = m[name] = new CPU;
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

void Processor::copy(std::map<std::string, CPU *> &dst,
                     std::map<std::string, CPU *> &src) {
    for (const auto &kv: src) {
        CPU *o = (CPU *) kv.second;
        const char *name = o->name;

        CPU *cpu = dst[name];
        if (!cpu) {
            cpu = dst[name] = new CPU;
            cpu->name = strdup(name);
        }
        cpu->user = o->user;
        cpu->system = o->system;
        cpu->idle = o->idle;
        cpu->nice = o->nice;
    }
}

void Processor::update() {
    this->copy(this->last, this->current);
    this->read(this->current);
    for (const auto &kv: this->delta) {
        CPU *cpu = (CPU *) kv.second;
        cpu->diff(this->current[cpu->name], this->last[cpu->name]);
    }
    CPU *cpu = this->delta["CPU"];
    cpu->user /= this->num_cores;
    cpu->system /= this->num_cores;
    cpu->nice /= this->num_cores;
    cpu->idle /= this->num_cores;
}

uint16_t Processor::print() {
    uint16_t count = 0;
    CPU *cpu;

    console.inverseln("  %-6s %7s %7s %7s %7s %7s ", "[C]PUS", "USE", "User",
                      "System", "Nice", "Idle");
    count++;

    cpu = this->delta["CPU"];
    cpu->print();
    count++;
    if (!options.condenseCPU) {
        for (int i = 0; i < this->num_cores; i++) {
            char name[32];
            sprintf(name, "CPU%d", i);
            cpu = this->delta[name];
            cpu->print();
            count++;
        }
    }
    return count;
}

Processor processor;

