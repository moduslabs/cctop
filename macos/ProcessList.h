//
// Created by Michael Schwartz on 11/23/21.
//

#ifndef CCTOP_PROCESSLIST_H
#define CCTOP_PROCESSLIST_H

#include <map>
#include <string>
#include <sys/mount.h>

struct Process {
    uint64_t delta_cpu;
    double pct_cpu;
    uint32_t flags;
    uint32_t status;
    uint32_t exit_status;
    uint32_t pid;
    uint32_t ppid;
    uid_t uid;
    gid_t gid;
    uid_t ruid;
    gid_t rgid;
    uid_t svuid;
    gid_t svgid;
    char comm[MAXCOMLEN + 1];
    char name[2 * MAXCOMLEN + 1];
    uint32_t nfiles;
    uint32_t pgid;
    uint32_t pjobc;
    uint32_t e_tdev;
    uint32_t e_tpgid;
    uint64_t start_sec, start_usec;
    int32_t nice;

    uint64_t virtual_size;       /* virtual memory size (bytes) */
    uint64_t resident_size;      /* resident memory size (bytes) */
    uint64_t total_user;         /* total time */
    uint64_t delta_user;
    uint64_t total_system;
    uint64_t delta_system;
    uint64_t threads_user;       /* existing threads only */
    uint64_t threads_system;
    int32_t policy;             /* default policy for new threads */
    int32_t faults;             /* number of page faults */
    int32_t pageins;            /* number of actual pageins */
    int32_t cow_faults;         /* number of copy-on-write faults */
    int32_t messages_sent;      /* number of messages sent */
    int32_t messages_received;  /* number of messages received */
    int32_t syscalls_mach;      /* number of mach system calls */
    int32_t syscalls_unix;      /* number of unix system calls */
    int32_t csw;                /* number of context switches */
    int32_t threadnum;          /* number of threads in the task */
    int32_t numrunning;         /* number of running threads */
    int32_t priority;           /* task priority*/
};

class ProcessList {
public:
    ProcessList();

    ~ProcessList();

public:
    void update();

    void print(bool test);

protected:
    std::map<int, Process *> list;
    std::map<uid_t, std::string *> uids;
    std::map<gid_t, std::string *> gids;

public:
    const char *username(uid_t uid) {
        static char buf[128];
        sprintf(buf, "%d", uid);
        return uids.count(uid) == 0 ? buf : uids[uid]->c_str();
    };

    const char *groupname(gid_t gid) {
        static char buf[128];
        sprintf(buf, "%d", gid);
        return gids.count(gid) == 0 ? buf : gids[gid]->c_str();
    };
};

extern ProcessList processList;

#endif //CCTOP_PROCESSLIST_H
