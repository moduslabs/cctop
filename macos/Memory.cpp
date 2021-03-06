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
 *
 * This file is based upon httpL//github.com/sklinkert/mac-iostat
 * which is based upon Apple's iostat.c
 * which is based upon BSD's iostat.c
 *
 * See copyright info in iostat.txt.
 */

#include "../cctop.h"
#include <mach/mach.h>
#include <unistd.h>

Memory::Memory() {
    this->page_size = static_cast<uint64_t>(sysconf(_SC_PAGESIZE));

    this->read(&this->last);
    this->update();
}

void Memory::read(MemoryStats *stats) {
    mach_msg_type_number_t count = HOST_VM_INFO64_COUNT;
    vm_statistics64_data_t vmstat;
    host_statistics64(mach_host_self(), HOST_VM_INFO64, (host_info64_t) &vmstat, &count);
    stats->free_count = vmstat.free_count;
    stats->active_count = vmstat.active_count;
    stats->inactive_count = vmstat.inactive_count;
    stats->wire_count = vmstat.wire_count;
    stats->external_page_count = vmstat.external_page_count;
    stats->purgeable_count = vmstat.purgeable_count;
    stats->pageins = vmstat.pageins;
    stats->pageouts = vmstat.pageouts;
    stats->faults = vmstat.faults;
    stats->swapins = vmstat.swapins;
    stats->swapouts = vmstat.swapouts;

    int vmmib[2] = {CTL_VM, VM_SWAPUSAGE};
    xsw_usage swapused; /* defined in sysctl.h */
    size_t swlen = sizeof(swapused);
    sysctl(vmmib, 2, &swapused, &swlen, nullptr, 0);

    stats->total_pages =
            vmstat.wire_count + vmstat.active_count + vmstat.inactive_count + vmstat.free_count +
            vmstat.compressor_page_count;

    stats->memory_size = stats->total_pages * page_size;

    stats->memory_used = page_size *
                         (vmstat.wire_count + vmstat.active_count + vmstat.compressor_page_count +
                          vmstat.inactive_count);

    stats->memory_free = stats->memory_size - stats->memory_used - vmstat.inactive_count;

    stats->swap_size = swapused.xsu_total;
    stats->swap_used = swapused.xsu_used;
    stats->swap_free = swapused.xsu_avail;
}

void Memory::update() {
    MemoryStats *current = &this->current,
            *last = &this->last,
            *delta = &this->delta;

    this->read(&this->current);

    delta->free_count = current->free_count - last->free_count;
    delta->active_count = current->active_count - last->active_count;
    delta->inactive_count = current->inactive_count - last->inactive_count;
    delta->wire_count = current->wire_count - last->wire_count;
    delta->pageins = current->pageins - last->pageins;
    delta->pageouts = current->pageouts - last->pageouts;
    delta->faults = current->faults - last->faults;
    delta->swapins = current->swapins - last->swapins;
    delta->swapouts = current->swapouts - last->swapouts;

    delta->swap_size = current->swap_size - last->swap_size;
    delta->swap_used = current->swap_used - last->swap_used;
    delta->swap_free = current->swap_free - last->swap_free;
}

uint16_t Memory::print(bool newline) const {
    uint16_t count = 0;
    console.inverseln("%-12s   %9s %9s %9s %9s %9s",
                      "  [M]EMORY", "Total", "Used", "Free", "Wired", "Cached");
    count++;

#if 0
    char total[200], used[200], free[200], wired[200], cached[200];
    console.println("  %-12s %9s %9s %9s %9s %9s",
                    "Real",
                    console.humanSize(this->current.memory_size, total),
                    console.humanSize(this->current.memory_used, used),
                    console.humanSize(this->current.memory_free, free),
                    console.humanSize(this->current.wire_count, wired),
                    console.humanSize(this->page_size * (this->current.external_page_count + this->current.purgeable_count)), cached);
#endif
    uint64_t cached = page_size * (current.external_page_count + current.purgeable_count);
    double pct = (double(current.memory_used) - double(cached)) / double(current.memory_size);
    console.print("  %-12s %'9lld %'9lld %'9lld %'9lld %'9lld ",
                  "Real",
                  current.memory_size / 1024 / 1024,
                  current.memory_used / 1024 / 1024, current.memory_free / 1024 / 1024,
                  current.wire_count,
                  page_size * (current.external_page_count + current.purgeable_count) / 1024 / 1024);
    console.gauge(20, pct*100, 1);
    console.newline();
    count++;

    if (!options.condenseMemory) {
        console.println("  %-12s %'9lld %'9lld %'9lld", "Swap", this->current.swap_size / 1024 / 1024,
                        this->current.swap_used / 1024 / 1024, this->current.swap_free / 1024 / 1024);
    }
    if (newline) {
        console.newline();
        count++;
    }
    return count; // # lines printed
}

uint16_t Memory::printVirtualMemory(bool newline) {
    uint16_t count = 0;

    console.inverseln("  %-16s %19s %22s", "[V]IRTUAL MEMORY", "  IN Current OUT  ", "  IN Aggregate OUT ");
    count++;
    if (!options.condenseVirtualMemory) {
        console.println("  %-12s %'9lld   %'9lld %'9lld     %'9lld", "Page",
                        this->delta.pageins / 1024 / 1024,
                        this->delta.pageouts / 1024 / 1024,
                        this->current.pageins / 1024 / 1024,
                        this->current.pageouts / 10924 / 1024);
        count++;
        console.println("  %-12s %'9lld   %'9lld %'9lld     %'9lld", "Swap",
                        this->delta.swapins / 1024 / 1024,
                        this->delta.swapouts / 1024 / 1024,
                        this->current.swapins / 1024 / 1024,
                        this->current.swapouts / 10924 / 1024);
        count++;
    }
    if (newline) {
        console.newline();
        count++;
    }
    return count;
}

Memory memory;
