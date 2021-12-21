//
// Created by Michael Schwartz on 11/23/21.
//

#include "../cctop.h"
#include <libproc.h>
#include <vector>
#include <utility>

ProcessList::ProcessList() {
    //
}

ProcessList::~ProcessList() {
    //
    list.clear();
}

static pid_t pids[99999];

void ProcessList::update() {
    touched++; // bump so we know which in list<> we've seen.

    int num_processes = proc_listallpids(pids, sizeof(pids));
    for (int pp = 0; pp < num_processes; pp++) {
        pid_t pid = pids[pp];
        Process *p;
        bool isNew = false;
        if (list.count(pid) == 0) {
            p = new Process();
            p->touched = 0;
            list.emplace((const uint32_t) pid, p);
            isNew = true;
        } else {
            p = list[pid];
        }

        proc_bsdinfo proc{};
        int ret = proc_pidinfo(pid, PROC_PIDTBSDINFO, 0, &proc, sizeof(proc));
        if (ret == 0) {
            list.erase(pid);
            continue;
        }
        p->flags = proc.pbi_flags;
        p->status = proc.pbi_status;
        p->exit_status = proc.pbi_xstatus;
        p->pid = proc.pbi_pid;
        p->ppid = proc.pbi_ppid;
        p->uid = proc.pbi_uid;
        p->gid = proc.pbi_gid;
        p->ruid = proc.pbi_ruid;
        p->rgid = proc.pbi_rgid;
        p->svuid = proc.pbi_svuid;
        p->svgid = proc.pbi_svgid;
        strcpy(p->comm, proc.pbi_comm);
        strcpy(p->name, proc.pbi_name);
        p->nfiles = proc.pbi_nfiles;
        p->pgid = proc.pbi_pgid;
        p->pjobc = proc.pbi_pjobc;
        p->e_tdev = proc.e_tdev;
        p->e_tpgid = proc.e_tpgid;
        p->start_sec = proc.pbi_start_tvsec;
        p->start_usec = proc.pbi_start_tvusec;
        p->nice = proc.pbi_nice;

        proc_taskinfo info{};
        proc_pidinfo(pid, PROC_PIDTASKINFO, 0, &info, sizeof(info));
        p->virtual_size = info.pti_virtual_size;
        p->resident_size = info.pti_resident_size;
        if (isNew) {
            p->delta_system = 0;
            p->delta_user = 0;
        } else {
            p->delta_system = info.pti_total_system - p->total_system;
            p->delta_user = info.pti_total_user - p->total_user;
        }
        p->touched = touched;
        p->delta_cpu = p->delta_system + p->delta_user;
        if (processor.total_ticks > 0) {
            p->pct_cpu = double(p->delta_cpu) / double(processor.total_ticks);
        } else {
            p->pct_cpu = 0.;
        }
        p->total_user = info.pti_total_user;
        p->total_system = info.pti_total_system;
        p->threads_user = info.pti_threads_user;
        p->threads_system = info.pti_threads_system;
        p->policy = info.pti_policy;
        p->faults = info.pti_faults;
        p->pageins = info.pti_pageins;
        p->cow_faults = info.pti_cow_faults;
        p->messages_sent = info.pti_messages_sent;
        p->messages_received = info.pti_messages_received;
        p->syscalls_mach = info.pti_syscalls_mach;
        p->syscalls_unix = info.pti_syscalls_unix;
        p->csw = info.pti_csw;
        p->threadnum = info.pti_threadnum;
        p->numrunning = info.pti_numrunning;
        p->priority = info.pti_priority;

#if 0
        if (uids.count(proc.pbi_uid) == 0) {
            passwd *pass = getpwuid(proc.pbi_uid);
            uids.emplace(proc.pbi_uid, new std::string(pass->pw_name));
        }
        if (gids.count(proc.pbi_gid) == 0) {
            group *grp = getgrgid(proc.pbi_gid);
            gids.emplace(proc.pbi_gid, new std::string(grp->gr_name));
        }
#endif
    }
}

static bool cmp(Process *a, Process *b) {
    return a->pct_cpu > b->pct_cpu;
}

uint16_t ProcessList::print(bool newline) {
    uint16_t count = 0;
    std::vector<Process *> sorted, to_remove;
    // Loop through our map of processes (pid is key, value is struct).
    //
    // If the process' touched value is not up-to-date with the master one
    // in processList, then it's no longer running and needs to be removed;
    // we add it to the to_remove vector.
    //
    // Otherwise, we add it to the sorted vector.
    //
    int remove_count = 0;
    for (auto &it: list) {
        auto p = it.second;
        if (p->touched != touched) {
            to_remove.push_back(p);
            remove_count++;
        } else {
            sorted.push_back(p);
        }
    }
    // We now have two vectors - sorted (to be sorted) and to_remove (to be removed).
    // Loop through to_remove and remove the Process structs from list.
    for (auto &it: to_remove) {
        auto p = it;
        list.erase(p->pid);
    }

    // sort away
    std::sort(sorted.begin(), sorted.end(), cmp);
    int printed = 0;
    console.inverseln(" %6.6s %6.6s %-16.16s %-32.32s", "[P]ID", "CPU%", "USER", "NAME");
    count++;
    int lines = console.height - console.cursor_row() -2;
    for (auto &it: sorted) {
        auto p = it;
//        if (p->ppid > 1) continue;
        printed++;
        auto pass = username(p->ruid);
        auto grp = groupname(p->rgid);

        if (!strcmp(p->name, "cctop")) {
            console.mode_bold(true);
        }
        console.println(" %6d %6.1f %-16.16s %-32.32s", p->pid, p->pct_cpu * 1000, pass, p->name);
        console.mode_bold(false);
        count++;
        if (options.condenseProcesses) {
            break;
        }
        if (printed > lines) break;
    }
    console.println("  %ld processes", sorted.size());
    count++;
    if (newline) {
        console.newline();
        count++;
    }
    return count;
}

ProcessList processList;