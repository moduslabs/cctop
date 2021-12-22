#include "cctop.h"
#include <clocale>
#include <unistd.h>

#pragma clang diagnostic push
#pragma ide diagnostic ignored "EndlessLoop"

void resize_help() {
#if !debug
    console.clear();
    console.println("Window is %dx%d and needs to be at least %dx%d]",
                    console.width, console.height,
                    MIN_WIDTH, MIN_HEIGHT);
#endif
}

int required_lines = -1;

uint16_t loop() {
    if (console.width < MIN_WIDTH || console.height < MIN_HEIGHT) {
//        debug.log("resize_help");
        resize_help();
        return 0;
    }
    int lines = 0;

    platform.update();
    processor.update();
    memory.update();
    disk.update();
    network.update();
    processList.update();

    console.moveTo(0, 0);
    console.mode_clear();

#ifndef USE_NCURSES
    if (!options.showHelp) {
#endif
    bool condense = options.condenseMain;
    options.condenseCPU = options.condenseCPU_state;
    if (console.height < 40) {
        options.condenseCPU = true;
        if (console.height < 35) {
            condense = true;
        }
    } else if (console.height < 45) {
        condense = true;
    }
//    debug.log("window %d x %d %d\n", console.width, console.height, condense);
    lines += platform.print(!condense);
    lines += processor.print(!condense);
    lines += memory.print(!condense);
    lines += memory.printVirtualMemory(!condense);
    lines += disk.print(!condense);
    lines += network.print(!condense);
    lines += processList.print(!condense);
#ifndef USE_NCURSES
    }
#endif

    if (required_lines == -1) {
        required_lines = lines;
    }
    Help::show();
    lines++;
//    console.print("%d/%d lines %dx%d\n", lines, required_lines, console.width, console.height);
//    console.clear(true);
    return lines;
}

int main() {
    setlocale(LC_ALL, "");
    console.clear();
    console.raw();

#if __APPLE__
    uid_t uid = geteuid();
    if (uid != 0) { // not root's UID
        console.moveTo(0,0);
        console.print("*** Warning: This program should be run as root, or via sudo!\n");
        console.print("    Otherwise, only your user processes can be examined.\n");
        console.print("    Do you wish to continue anyway? (y/N): ");
        int c;
        if (!console.read_character(&c, false)) {
            exit(0);
        }
        if (c == 'N' || c == 'n') {
            exit(0);
        }
    }
#endif

#if 0
    //    2581 ▁
    //    2582 ▂
    //    2583 ▃
    //    2584 ▄
    //    2585 ▅
    //    2586 ▆
    //    2587 ▇
    //    2588 █
        const wchar_t dots[] = {
                L'▁', // 0x2581
                L'▂', // 0x2582
                L'▃', // 0x2583
                L'▄', // 0x2584
                L'▅', // 0x2585
                L'▆', // 0x2586
                L'▇', // 0x2587
                L'█', // 0x2588
        };
        console.print("\nres: %d\n", res);
    //    wchar_t c = L'⣿';
        wchar_t c = L'■';
        for (wchar_t cc = c - 100; cc < c + 100; cc++) {
            wprintf(L"%04x %lc\n", cc, cc);
        }

        wprintf(L"\n\n%p %lc\n", c, c);

        const wchar_t dots4 = 0x28ff,
                dots3 = 0x28f6,
                dots2 = 0x28e4,
                dots1 = 0x28c0,
                dots0 = ' ';

        for (int i = 0; i < 10; i++) {
            wprintf(L"%lc%lc%lc%lc", dots4, dots4, dots4, dots4);
            wprintf(L"%lc%lc%lc%lc", dots3, dots3, dots3, dots3);
            wprintf(L"%lc%lc%lc%lc", dots2, dots2, dots2, dots2);
            wprintf(L"%lc%lc%lc%lc", dots1, dots1, dots1, dots1);
            wprintf(L"%lc%lc%lc%lc", dots0, dots0, dots0, dots0);
        }
        printf("\n");
#else

    wchar_t b = L'■';
    int c;
    console.raw(true);
    console.show_cursor(false);
    loop();
    for (;;) {
        loop();
        console.update();
        if (console.read_character(&c, true)) {
            options.process(c);
        }
    }
#endif
    return 0;
}

#pragma clang diagnostic pop
