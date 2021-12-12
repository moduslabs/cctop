/**
 * Not used currently.
 *
 * The implementation should figure out which processes are using power.
 */
#if 0
#include "Battery.h"
#include <mach/mach_init.h>
#include <mach/task.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

static void Error(const char *aMsg) {
    fprintf(stderr, "error: %s\n", aMsg);
    exit(1);
}

static bool GetTaskPowerInfo(struct task_power_info *aInfo) {
    mach_msg_type_number_t count = TASK_POWER_INFO_COUNT;
    kern_return_t kr =
            task_info(mach_task_self(), TASK_POWER_INFO, (task_info_t) aInfo, &count);
    return kr == KERN_SUCCESS;
}


void Battery::update() {
    task_power_info info{};
    if (!GetTaskPowerInfo(&info)) {
        return;
    }
    printf("here");
}

void Battery::print() {

}

Battery battery;
#endif