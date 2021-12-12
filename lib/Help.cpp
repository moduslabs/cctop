#include "Help.h"
#include "Console.h"
#include "Options.h"

static const char *true_false(bool t) {
    return t ? "[ TRUE ]" : "[ FALSE ]";
}

void Help::show() {
    if (options.showHelp) {
        int margin = 2, padding = 2;
        console.window(5, margin + 1,
                       console.width - margin - margin, 14,
                       "HELP");
        int row = margin + 1 + padding,
                col = margin + 1 + padding;

//        console.moveTo(row++, col);
//        console.print("HELP");
        row++;
        row++;

        console.moveTo(row++, col);
        console.print("C %-48.48s %s", "toggles condensed CPU display", true_false(options.condenseCPU));
        console.moveTo(row++, col);
        console.print("M %-48.48s %s", "toggles condensed Memory display", true_false(options.condenseMemory));
        console.moveTo(row++, col);
        console.print("V %-48.48s %s", "toggles condensed Virtual Memory display", true_false(options.condenseVirtualMemory));
        console.moveTo(row++, col);
        console.print("D %-48.48s %s", "toggles condensed Disk Activity display", true_false(options.condenseDisk));
        console.moveTo(row++, col);
        console.print("N %-48.48s %s", "toggles condensed Network display", true_false(options.condenseNetwork));
        console.moveTo(row++, col);
        console.print("P %-48.48s %s", "toggles condensed Process List display", true_false(options.condenseProcesses));

        console.moveTo(row++, col);
        console.print("X %-48.48s %s", "toggles remove blank lines", true_false(options.condenseMain));
//        console.moveTo(row++, col);
//        console.print("^L to refresh");

        row++;
        console.moveTo(row++, col);
        console.print("H toggles this Help Window");

        row++;
        console.moveTo(row++, col);
        console.print("Q, ^D,  or ^C to quit");

        row++;
        row++;
        row++;
        console.moveTo(row++, col);
//        row++;
//        row++;
//        console.moveTo(row++, col);
//        console.print("Any key to continue");

//        int c;
//        if (console.getch(&c, false)) {
//            options.showHelp = false;
//        }
    }
}

Help help;