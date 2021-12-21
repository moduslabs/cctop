/*
 * cctop for MacOS and Linux
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

// General purpose console class
// Uses ANSI escape sequences to all for interactive console UI.
// Does not rely on any dependency, such as curses/ncurses (that's the point!)

// TODO: stdin, raw, echo, icanon, etc.
// https://viewsourcecode.org/snaptoken/kilo/02.enteringRawMode.html

#include "../cctop.h"
#include "Console.h"
#include <ncurses.h>
#include "Options.h"
#include <cwchar>
#include <csignal>
#include <cstdarg>
#include <cstdlib>
#include <cstdio>
#include <unistd.h>

#ifdef USE_NCURSES


#else

#include <termios.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <ctime>

#endif

const uint8_t ATTR_OFF = 0;
const uint8_t ATTR_BOLD = 1;
const uint8_t ATTR_UNDERSCORE = 4;
const uint8_t ATTR_BLINK = 5;
const uint8_t ATTR_INVERSE = 7;

#ifdef USE_NCURSES
// colors
const uint8_t FG_BLACK = COLOR_BLACK;
const uint8_t FG_RED = COLOR_RED;
const uint8_t FG_GREEN = COLOR_GREEN;
const uint8_t FG_YELLOW = COLOR_YELLOW;
const uint8_t FG_BLUE = COLOR_BLUE;
const uint8_t FG_MAGENTA = COLOR_MAGENTA;
const uint8_t FG_CYAN = COLOR_CYAN;
const uint8_t FG_WHITE = COLOR_WHITE;
const uint8_t BG_BLACK = COLOR_BLACK;
const uint8_t BG_RED = COLOR_RED;
const uint8_t BG_GREEN = COLOR_GREEN;
const uint8_t BG_YELLOW = COLOR_YELLOW;
const uint8_t BG_BLUE = COLOR_BLUE;
const uint8_t BG_MAGENTA = COLOR_MAGENTA;
const uint8_t BG_CYAN = COLOR_CYAN;
const uint8_t BG_WHITE = COLOR_WHITE;

const uint8_t DEFAULT_BACKGROUND = COLOR_BLACK;
const uint8_t DEFAULT_FOREGROUND = COLOR_WHITE;

#else
const char ESC = 0x1b;

// colors
const uint8_t FG_BLACK = 30;
const uint8_t FG_RED = 31;
const uint8_t FG_GREEN = 32;
const uint8_t FG_YELLOW = 33;
const uint8_t FG_BLUE = 34;
const uint8_t FG_MAGENTA = 35;
const uint8_t FG_CYAN = 36;
const uint8_t FG_WHITE = 37;
const uint8_t BG_BLACK = 40;
const uint8_t BG_RED = 41;
const uint8_t BG_GREEN = 42;
const uint8_t BG_YELLOW = 43;
const uint8_t BG_BLUE = 44;
const uint8_t BG_MAGENTA = 45;
const uint8_t BG_CYAN = 46;
const uint8_t BG_WHITE = 47;

const uint8_t DEFAULT_BACKGROUND = BG_BLACK;
const uint8_t DEFAULT_FOREGROUND = FG_WHITE;
#endif

#ifdef USE_NCURSES
#if 0
const int MAX_COLOR_PAIR = 256;

typedef struct {
    uint8_t fg;
    uint8_t bg;
    int count;
    bool set;
} ConsolePair;

static ConsolePair allocated_pairs[MAX_COLOR_PAIR] = {0};

static int free_color_pair(int pair) {
    if (pair < 1 || pair >= MAX_COLOR_PAIR || !(allocated_pairs[pair].set)) {
        return ERR;
    }

    allocated_pairs[pair].set = FALSE;
    return OK;
}

static void xinit_color_pairs() {
    ConsolePair *p = allocated_pairs;

    for (int i = 0; i < MAX_COLOR_PAIR; i++) {
        debug.log("i %d\n", i);
        p[i].set = false;
    }
}

static int find_color_pair(uint8_t fg, uint8_t bg) {
    int i;
    ConsolePair *p = allocated_pairs;

    for (i = 0; i < MAX_COLOR_PAIR; i++) {
        if (p[i].set && p[i].fg == fg && p[i].bg == bg) {
            p[i].count++;
//            debug.log(">> find_color_pair %d", i);
            return i;
        }
    }
//    debug.log("find_color_pair -1");
    return -1;
}

static int _find_oldest_pair() {
//    debug.log("** find_oldest_pair");
    int i, lowind = 0, lowval = 0;
    ConsolePair *p = allocated_pairs;

    for (i = 1; i < MAX_COLOR_PAIR; i++) {
        if (!p[i].set) {
            return i;
        }

        if (!lowval || (p[i].count < lowval)) {
            lowind = i;
            lowval = p[i].count;
        }
    }

    return lowind;
}

static int alloc_color_pair(uint8_t fg, uint8_t bg) {
    int i = find_color_pair(fg, bg);

    if (-1 == i) {
        i = _find_oldest_pair();
        ConsolePair *p = allocated_pairs;

        if (ERR == init_pair(short(i), fg, bg)) {
//            debug.log("*** can't init_pair(%d, %d, %d)\n", i, fg, bg);
            return -1;
        }
        p[i].set = true;
        p[i].fg = fg;
        p[i].bg = bg;
        p[i].count = 0;
    }
    debug.log("alloc_pair (%d, %d) %d\n", fg, bg, i);
    return i;

}
#endif
#endif

static long millis() {
    timeval time{};
    gettimeofday(&time, nullptr);
    return (time.tv_sec * 1000) + (time.tv_usec / 1000);
}

// window size/change handling
void resize_handler(int sig) {
    if (sig == SIGWINCH) {
        console.resize();
    } else {
        printf("signal %d\n", sig);
    }
}

void exit_handler(int sig) {
    console.cleanup();
    if (sig == SIGINT) {
        printf("^C\n");
    } else if (sig == SIGTERM) {
        printf("KILLED\n");
    }
    exit(0);
}

uint16_t Console::cursor_row() {
#ifdef USE_NCURSES
    current_row = getcury(stdscr);
#endif
    return current_row;
}

uint16_t Console::cursor_column() {
#ifdef USE_NCURSES
    current_column = getcury(stdscr);
#endif
    return current_column;
}

void Console::update() {
#ifdef USE_NCURSES
    refresh();
#endif
}

void Console::cleanup() {
    if (!aborting) {
#ifndef USE_NCURSES
        reset();
        clear();
        show_cursor(true);
        tcsetattr(0, TCSANOW, &initial_termios);
#else
        endwin();
#endif
    }
    aborting = true;
}

Console::Console() {
#ifdef USE_NCURSES
    initscr();
    start_color();
//    xinit_color_pairs();
#endif
    default_colors();

    aborting = false;
    resize();
    reset();
    clear();
#ifndef USE_NCURSES
    // install sigwinch handler (window resize signal)
    signal(SIGWINCH, resize_handler);
    tcgetattr(0, &initial_termios);
#endif
    signal(SIGINT, exit_handler);
    signal(SIGTERM, exit_handler);

}

Console::~Console() {
    cleanup();
}

void Console::abort(const char *fmt, ...) {
    va_list ap;

    va_start(ap, fmt);
    cleanup();
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fflush(stdout);
}

void Console::resize() {
#ifdef USE_NCURSES
    width = COLS;
    height = LINES;
#else
    winsize size{0};
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &size);
    width = 120;
    height = 120;
//    this->width = size.ws_col;
//    this->height = size.ws_row;
    clear();
#endif
    clear();
}

void Console::raw(bool on) {
#ifdef USE_NCURSES
    ::raw();
    cbreak();
    keypad(stdscr, true);
#else
    termios t{0};
    tcgetattr(0, &t);
    if (on) {
        // disable buffering and echo and signals (^c, etc.)
        t.c_lflag &= ~(ICANON | ECHO | ISIG);
        tcsetattr(0, TCSANOW, &t);
    } else {
        // enable buffering and echo and signals (^c, etc.)
        t.c_lflag |= (ICANON | ECHO | ISIG);
    }
#endif
    raw_input = on;
}

bool Console::read_character(int *c, bool timeout) {
    long now = millis(), when = now + options.read_timeout;
#ifndef USE_NCURSES
    char cc = '\0';
    if (timeout) {
        while (millis() < when) {
            fd_set set;
            FD_ZERO(&set);
            FD_SET(0, &set);

            timeval t{};
            t.tv_sec = 0;
            t.tv_usec = 100;

            int ret = select(1, &set, nullptr, nullptr, &t);
            switch (ret) {
                case -1:
                    // error occurred
                    return false;
                case 0:
                    // timeout
                    usleep(100);
                    continue;
                default:
                    if (read(0, &cc, 1) == 1) {
                        *c = int(cc);
                        return true;
                    }
                    break;
            }
        }
    }
#else
    ::timeout(500);

    while (millis() < when) {
        int cc = getch();
        switch (cc) {
            case 0:
            case ERR:
                continue;
            case KEY_RESIZE:
                resize();
                continue;
            default:
                *c = cc;
                return true;
        }
    }
#endif

    return false;
}

void Console::show_cursor(bool on) {
#ifdef USE_NCURSES
    if (on) {
        curs_set(1);
        cursor_hidden = false;
    } else {
        curs_set(0);
        cursor_hidden = true;
    }
#else
    if (on) {
        printf("%c[?25h", ESC, stdout);
        cursor_hidden = false;
    } else {
        printf("%c[?25l", ESC, stdout);
        cursor_hidden = true;
    }
    fflush(stdout);
#endif
}

/** @public **/
void Console::clear(bool endOfScreen) {
#ifdef USE_NCURSES
    ::clear();
#else
    printf("%c[%dJ", ESC, endOfScreen ? 0 : 2);
    fflush(stdout);
#endif
    moveTo(0, 0);
}

/** @public **/
void Console::reset() {
#ifdef USE_NCURSES
    attrset(A_NORMAL);
#else
    set_mode(ATTR_OFF, true);
    background = foreground = 0;
    show_cursor(true);
#endif
}

/** @public **/
void Console::clear_eol() {
#ifdef USE_NCURSES
    clrtoeol();
#else
    printf("%c[0K", ESC);
    fflush(stdout);
#endif
}

/** @public **/
void Console::moveTo(uint16_t r, uint16_t c) {
#ifdef USE_NCURSES
    move(r, c);
#else
    printf("%c[%d;%dH", ESC, row, col);
    fflush(stdout);
#endif
    current_row = r;
    current_column = c;
}

/** @private */
void Console::set_mode(uint8_t attr, bool on) {
#ifdef USE_NCURSES
    switch (attr) {
        case ATTR_OFF:
            if (on) {
                attron(A_NORMAL);
            } else {
                attroff(A_NORMAL);
            }
            bold = underscore = blink = inverse = false;
            break;
        case ATTR_BOLD:
            if (on) {
                attron(A_BOLD);
            } else {
                attroff(A_BOLD);
            }
            bold = on;
            break;
        case ATTR_UNDERSCORE:
            if (on) {
                attron(A_UNDERLINE);
            } else {
                attroff(A_UNDERLINE);
            }
            underscore = on;
            break;
        case ATTR_BLINK:
            if (on) {
                attron(A_BLINK);
            } else {
                attroff(A_BLINK);
            }
            blink = on;
            break;
        case ATTR_INVERSE:
            if (on) {
                attron(A_REVERSE);
            } else {
                attroff(A_REVERSE);
            }
            inverse = on;
            break;
        default:
            // shouldn't get here
            break;
    }
#else
    if (on) {
        printf("%c[%dm", ESC, attr);
    } else {
        printf("%c[=%dl", ESC, attr);
    }
    fflush(stdout);
    switch (attr) {
        case ATTR_OFF:
            bold = underscore = blink = inverse =
            concealed = false;
            show_cursor(!cursor_hidden);
            break;
        case ATTR_BOLD:
            bold = on;
            break;
        case ATTR_UNDERSCORE:
            underscore = on;
            break;
        case ATTR_BLINK:
            blink = on;
            break;
        case ATTR_INVERSE:
            inverse = on;
            break;
//        case ATTR_CONCEALED:
//            concealed = on;
//            break;
        default:
            break;
    }
#endif
}

void Console::mode_clear() {
#ifdef USE_NCURSES
//    default_colors();
    attrset(A_NORMAL);
#else
    set_mode(ATTR_OFF, true);
    fflush(stdout);
#endif
}

/** @public **/
void Console::mode_bold(bool on) { set_mode(ATTR_BOLD, on); }

/** @public **/
void Console::mode_underscore(bool on) { set_mode(ATTR_UNDERSCORE, on); }

/** @public **/
void Console::mode_blink(bool on) { set_mode(ATTR_BLINK, on); }

/** @public **/
void Console::mode_inverse(bool on) { set_mode(ATTR_INVERSE, on); }

void Console::set_colors(uint8_t fg, uint8_t bg) {
    foreground = fg;
    background = bg;
#ifdef USE_NCURSES
    current_pair = find_pair(fg, bg);
    if (current_pair != -1) {
        attron(COLOR_PAIR(current_pair));
    } else {
        current_pair = alloc_pair(foreground, background);
        attron(COLOR_PAIR(current_pair));
    }
#else
    printf("%c[%dm", ESC, foreground);
    printf("%c[%dm", ESC, background);
#endif
}

/** @private */
void Console::set_foreground(uint8_t color) {
    foreground = color;
#ifdef USE_NCURSES
    set_colors(foreground, background);
#else
    printf("%c[%dm", ESC, foreground);
#endif
}

void Console::set_background(uint8_t color) {
    background = color;
#ifdef USE_NCURSES
    set_colors(foreground, background);
#else
    printf("%c[%dm", ESC, background);
#endif
}

void Console::default_colors() {
    set_colors(DEFAULT_FOREGROUND, DEFAULT_BACKGROUND);
}

/** @public **/
void Console::colors_clear() {
    if (background) {
        set_background(DEFAULT_BACKGROUND);
    }
    if (foreground) {
        set_foreground(DEFAULT_FOREGROUND);
    }
}

/** @public **/
void Console::bg_clear() {
    if (background) {
        set_background(DEFAULT_BACKGROUND);
    }
    background = 0;
}

/** @public **/
void Console::fg_clear() {
    if (foreground) {
        set_foreground(DEFAULT_FOREGROUND);
    }
    foreground = 0;
}

/** @public **/
//void Console::fg_rgb(uint8_t red, uint8_t green, uint8_t blue) {
//    set_foreground(RGB(red, green, blue));
//}

/** @public **/
//void Console::bg_rgb(uint8_t red, uint8_t green, uint8_t blue) {
//    set_background(RGB(red, green, blue));
//}

/** @public **/
void Console::bg_black() {
    set_background(BG_BLACK);
}

/** @public **/
void Console::fg_black() {
    set_foreground(FG_BLACK);
}

/** @public **/
void Console::bg_red() {
    set_background(BG_RED);
}

/** @public **/
void Console::fg_red() {
    set_colors(FG_RED, this->background);
}

/** @public **/
void Console::bg_green() {
    set_background(BG_GREEN);
}

/** @public **/
void Console::fg_green() {
    set_foreground(FG_GREEN);
}

/** @public **/
void Console::bg_yellow() {
    set_colors(foreground, BG_YELLOW);
//    set_background(BG_YELLOW);
}

/** @public **/
void Console::fg_yellow() {
    set_colors(FG_YELLOW, background);
//    set_foreground(FG_YELLOW);
}

/** @public **/
void Console::bg_blue() {
    set_background(BG_BLUE);
}

/** @public **/
void Console::fg_blue() {
    set_foreground(FG_BLUE);
}

/** @public **/
void Console::bg_magenta() {
    set_background(BG_MAGENTA);
}

/** @public **/
void Console::fg_magenta() {
    set_foreground(FG_MAGENTA);
}

/** @public **/
void Console::bg_cyan() {
    set_background(BG_CYAN);
}

/** @public **/
void Console::fg_cyan() {
    set_foreground(FG_CYAN);
}

/** @public **/
void Console::bg_white() {
    set_background(BG_WHITE);
}

/** @public **/
void Console::fg_white() {
    set_foreground(FG_WHITE);
}

void Console::print(const char *fmt, ...) {
    va_list ap;
    char buffer[1024];

    va_start(ap, fmt);
    vsprintf(buffer, fmt, ap);
    va_end(ap);
#ifdef USE_NCURSES
    printw("%s", buffer);
#else
    printf("%s", buffer);
#endif
}

void Console::println(const char *fmt, ...) {
    va_list ap;

    char buffer[1024];
    va_start(ap, fmt);
    vsprintf(buffer, fmt, ap);
    va_end(ap);
#ifdef USE_NCURSES
    printw("%s", buffer);
#else
    printf("%s", buffer);
#endif
    newline();
}

void Console::inverseln(const char *fmt, ...) {
#ifdef USE_NCURSES
    char buffer[1024];
    va_list ap;

    va_start(ap, fmt);
    vsprintf(buffer, fmt, ap);
    va_end(ap);
    console.mode_inverse(true);
    printw("%s", buffer);
    int len = this->width - strlen(buffer) - 1;
    for (int i = 0; i < len; i++) {
        printw(" ");
    }
    console.mode_inverse(false);
#else
    va_list ap;

    va_start(ap, fmt);
//    console.mode_inverse(true);
    console.bg_white();
    console.fg_black();
    vprintf(fmt, ap);
    console.clear_eol();
    va_end(ap);
    fputs("\n", stdout);
    console.mode_clear();
    fflush(stdout);
#endif
    newline();
}

void Console::wprintf(const wchar_t *fmt, ...) {
#ifdef USE_NCURSES
    va_list ap;

    wchar_t buffer[1024];
    va_start(ap, fmt);
    vswprintf(buffer, 1024, fmt, ap);
    va_end(ap);
    ::addwstr((wchar_t *) buffer);
//    debug.printw(L"buffer: %ls\n", buffer);
#else
    va_list ap;

    va_start(ap, fmt);
    vwprintf(fmt, ap);
    va_end(ap);
    fflush(stdout);
#endif

}

void Console::wprintfln(const wchar_t *fmt, ...) {
    va_list ap;

    wchar_t buffer[1024];
    va_start(ap, fmt);
    vswprintf(buffer, 1024, fmt, ap);
    va_end(ap);
    debug.printw(L"addwstr %ls\n", buffer);
    ::addwstr((wchar_t *) buffer);
//    ::printw ((const wchar_t *)buffer);			/* generated:WIDEC */

    clear_eol();
    newline();
}

void Console::gauge(int aWidth, double pct, char fill) {
    print("[");
    int toFill = aWidth * pct / 100.,
            i;
    for (i = 0; i < toFill; i++) {
        print("%c", fill);
    }
    while (i < aWidth) {
        print(" ");
        i++;
    }
    print("]");
}

void Console::window(int aRow, int aCol, int aWidth, int aHeight, const char *aTitle) {
    int w = aWidth - 2;
    int h = aHeight - 2;
    int r = aRow;

    // top row
    moveTo(r++, aCol);
    wprintf(L"%lc", 0x2554);
    for (int i = 0; i < w; i++) {
        wprintf(L"%lc", 0x2550);
    }
    wprintf(L"%lc", 0x2557);

    // middle rows
    for (int hh = 0; hh < h; hh++) {
        moveTo(r++, aCol);
        wprintf(L"%lc", 0x2551);
        for (int i = 0; i < w; i++) {
            wprintf(L"%lc", ' ');
        }
        wprintf(L"%lc", 0x2551);
    }
    // bottom row
    moveTo(r++, aCol);
    wprintf(L"%lc", 0x255a);
    for (int i = 0; i < w; i++) {
        wprintf(L"%lc", 0x2550);
    }
    wprintf(L"%lc", 0x255d);

    if (aTitle) {
        moveTo(aRow, aCol + 2);
        print("[%s]", aTitle);
    }
}

void Console::newline(bool erase) {
    if (erase) {
        clear_eol();
    }
#ifdef USE_NCURSES
    printw("\n");
#else
    printf("\n");
    fflush(stdout);
#endif
}

Console console;
