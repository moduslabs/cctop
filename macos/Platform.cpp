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

#include "cctop.h"
#include <libproc.h>
#include <sys/utsname.h>
#include <CoreFoundation/CFString.h>
#include <CoreFoundation/CFNumber.h>
#include <mach/mach_init.h>
#include <mach/task.h>
#include <IOKit/ps/IOPowerSources.h>
#include <IOKit/ps/IOPSKeys.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string.h>
#include <ctime>
#include <unistd.h>

static bool get_task_power_info(struct task_power_info *aInfo) {
    mach_msg_type_number_t count = TASK_POWER_INFO_COUNT;
    kern_return_t kr =
            task_info(mach_task_self(), TASK_POWER_INFO, (task_info_t) aInfo, &count);
    return kr == KERN_SUCCESS;
}

static uint64_t get_uptime() {
    timeval boottime;
    size_t len = sizeof(boottime);
    int mib[2] = {CTL_KERN, KERN_BOOTTIME};
    if (sysctl(mib, 2, &boottime, &len, nullptr, 0) < 0) {
        return static_cast<uint64_t>(-1);
    }
    time_t bsec = boottime.tv_sec, csec = time(nullptr);

    return static_cast<uint64_t>(difftime(csec, bsec));
}

Platform::Platform() {
    this->refresh_time = 1;
    int cpuCount;
    int mib[2U] = {CTL_HW, HW_NCPU};
    size_t sizeOfCpuCount = sizeof(cpuCount);
    int status = sysctl(mib, 2U, &cpuCount, &sizeOfCpuCount, nullptr, 0U);
    assert(status == KERN_SUCCESS);
    this->cpu_count = static_cast<uint64_t>(cpuCount);

    utsname buf;
    uname(&buf);

    this->hostname = strdup(buf.nodename);
    this->sysname = strdup(buf.sysname);
    this->release = strdup(buf.release);
    this->version = strdup(buf.version);
    this->machine = strdup(buf.machine);

    this->kp = nullptr;
}

/*
 * You'll want to use IOKit for this, specifically the IOPowerSources functions.
 * You can use IOPSCopyPowerSourcesInfo() to get a blob, and IOPSCopyPowerSourcesList()
 * to then extract a CFArray out of that, listing the power sources. Then use
 * IOPSGetPowerSourceDescription() to pull out a dictionary (see IOPSKeys.h for the
 * contents of the dictionary).
 */
static double updateBattery() {
    CFTypeRef sourceInfo = IOPSCopyPowerSourcesInfo();
    CFArrayRef sourceList = IOPSCopyPowerSourcesList(sourceInfo);

    // Loop through sources, find the first battery
    int count = CFArrayGetCount(sourceList);
    CFDictionaryRef source = nullptr;
    for (int i = 0; i < count; i++) {
        source = IOPSGetPowerSourceDescription(sourceInfo, CFArrayGetValueAtIndex(sourceList, i));

        // Is this a battery?
        auto type = (CFStringRef) CFDictionaryGetValue(source, CFSTR(kIOPSTransportTypeKey));
        if (kCFCompareEqualTo == CFStringCompare(type, CFSTR(kIOPSInternalType), 0)) {
            break;
        }
    }

    double percent = 0;
    if (source != nullptr) {
        int curCapacity;
//        auto cr = CFDictionaryGetValue(source, CFSTR(kIOPSCurrentCapacityKey));
//        auto v = CFNumberGetValue((CFNumberRef)cr, kCFNumberIntType, &curCapacity);
        CFNumberGetValue((CFNumberRef) CFDictionaryGetValue(source, CFSTR(kIOPSCurrentCapacityKey)), kCFNumberIntType,
                         &curCapacity);

        int maxCapacity;
        CFNumberGetValue((CFNumberRef) CFDictionaryGetValue(source, CFSTR(kIOPSMaxCapacityKey)), kCFNumberIntType,
                         &maxCapacity);

        percent = curCapacity / (double) maxCapacity * 100.f;
    }

    CFRelease(sourceInfo);
    CFRelease(sourceList);
    return percent;
}

void Platform::update() {
    batteryInfo.chargePct = updateBattery();
    CFTypeRef r = IOPSCopyPowerSourcesInfo();
    CFArrayRef a = IOPSCopyPowerSourcesList(r);
    CFDictionaryRef d = IOPSGetPowerSourceDescription(a, kIOPSPowerAdapterWattsKey);
    CFRelease(a);
    IOPSGetProvidingPowerSourceType(r);
    CFStringGetCString(IOPSGetProvidingPowerSourceType(r), batteryInfo.powerSource, 128, kCFStringEncodingUTF8);
//    strcpy(&batteryInfo.powerSource[0], CFStringGetStringPtr(IOPSGetProvidingPowerSourceType(r)));
    switch (IOPSGetBatteryWarningLevel()) {
        case kIOPSLowBatteryWarningNone:
            batteryInfo.warningLevel = BatteryInfo::WARNING_NONE;
            break;
        case kIOPSLowBatteryWarningEarly:
            batteryInfo.warningLevel = BatteryInfo::WARNING_EARLY;
            break;
        case kIOPSLowBatteryWarningFinal:
            batteryInfo.warningLevel = BatteryInfo::WARNING_FINAL;
            break;
    }
    batteryInfo.timeRemaining = IOPSGetTimeRemainingEstimate();
    batteryInfo.powerState = BatteryInfo::POWER_BATTERY;
    if (batteryInfo.timeRemaining == kIOPSTimeRemainingUnlimited) {
        batteryInfo.powerState = BatteryInfo::POWER_PLUGGED_IN;
    } else if (batteryInfo.timeRemaining == kIOPSTimeRemainingUnknown) {
        batteryInfo.powerState = BatteryInfo::POWER_UNPLUGGED;
    }
//    task_power_info power_info{};
//    get_task_power_info(&power_info);
    getloadavg(this->loadavg, 3);
    size_t length = 0;
    static int names[] = {CTL_KERN, KERN_PROC, KERN_PROC_ALL, 0};
    sysctl((int *) names, (sizeof(names) / sizeof(names[0])) - 1, nullptr, &length,
           nullptr, 0);
    char *buf = new char[length];
    sysctl((int *) names, (sizeof(names) / sizeof(names[0])) - 1, (void *) buf,
           &length, nullptr, 0);

    delete[] this->kp;

    this->kp = (kinfo_proc *) buf;
    this->num_processes = length / sizeof(kinfo_proc);
//    for (int i = 0; i < this->num_processes; i++) {
//        kinfo_proc *p = &this->kp[i];
//        if (p->kp_proc.p_pctcpu > 0) {
//            printf("p");
//        }
//    }
}

uint16_t Platform::print(bool test) {
    this->uptime = get_uptime();
    if (test) {
        return 3;
    }
    time_t now = time(0);
    struct tm *p = localtime(&now);

    char s[1000];
    strftime(s, 1000, "%c", p);

    // compute current_uptime
    const int secs_per_day = 60 * 60 * 24, secs_per_hour = 60 * 60;
    uint64_t current_uptime = uptime;
    uint64_t days = current_uptime / secs_per_day;
    current_uptime -= days * secs_per_day;
    uint64_t hours = current_uptime / secs_per_hour;
    uint64_t minutes = (current_uptime - hours * secs_per_hour) / 60;

    const int width = console.width ? console.width : 80;
    char out[width + 1];
    sprintf(out, "cctop/%d [%s/%s %s]", refresh_time, hostname, sysname, release);
    size_t fill = width - strlen(out) - strlen(s) - 1;
    char *ptr = &out[strlen(out)];
    while (fill > 0) {
        *ptr++ = ' ';
        fill--;
    }
    strcat(ptr, s);
    console.inverseln(out);
    //  console.inverseln("cctop/%d [%s] %s/%s(%s) %s", 1, this->hostname,
    //  this->sysname, this->release, this->machine, s);

    console.mode_bold(true);
    console.print("Uptime: ");
    console.mode_clear();
    console.print("%d days %d:%02d  ", days, hours, minutes);
    console.mode_bold(true);
    console.print("Load Average: ");
    console.mode_clear();
    console.print("%5.2f %5.2f %5.2f", this->loadavg[0], this->loadavg[1],
                  this->loadavg[2]);
    console.clear_eol();
    console.newline();
    console.mode_bold(true);
    console.print("Power Source: ");
    console.mode_clear();
    console.print("%s ", batteryInfo.powerSource);
    console.mode_bold(true);
    console.print("Battery: ");
    console.mode_clear();
    console.print("%.0f%% ", batteryInfo.chargePct);
    console.gauge(20, batteryInfo.chargePct);
    if (batteryInfo.timeRemaining > 0) {
        console.print(" %.1f hours remaining", batteryInfo.hoursRemaining());
    }
    else if (batteryInfo.timeRemaining == -1) {
        console.print(" Caluclating time remaining");
    }
    console.clear_eol();
    console.newline();
    return 4;
}

Platform platform;
