#include "cctop.h"
#include <clocale>


#pragma clang diagnostic push
#pragma ide diagnostic ignored "EndlessLoop"

void resize_help() {
    console.clear();
    console.println("Window is %dx%d and needs to be at least %dx%d]",
                    console.width, console.height,
                    MIN_WIDTH, MIN_HEIGHT);
}

int required_lines = -1;

uint16_t loop() {
    if (console.width < MIN_WIDTH || console.height < MIN_HEIGHT) {
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

    console.moveTo(1, 1);
    lines += platform.print();
    console.newline();
    lines++;

    if (!options.showHelp) {
        lines += processor.print();
        if (lines < required_lines && !options.condenseMain) {
            console.newline();
            lines++;
        }

        lines += memory.print();
        if (lines < required_lines && !options.condenseMain) {
            console.newline();
            lines++;
        }
        lines += memory.printVirtualMemory();
        if (lines < required_lines && !options.condenseMain) {
            console.newline();
            lines++;
        }


        lines += disk.print();
        if (lines < required_lines && !options.condenseMain) {
            console.newline();
            lines++;
        }

        lines += network.print();
        if (lines < required_lines && !options.condenseMain) {
            console.newline();
            lines++;
        }

        lines += processList.print();
    }

    if (required_lines == -1) {
        required_lines = lines;
    }
    Help::show();
    lines++;
    console.print("%d/%d lines %dx%d\n", lines, required_lines, console.width, console.height);
    console.clear(true);
    return lines;
}

int main() {
    setlocale(LC_ALL, "");

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
        if (console.getch(&c, true)) {
            options.process(c);
        }
    }
#endif
    return 0;
}

#pragma clang diagnostic pop
