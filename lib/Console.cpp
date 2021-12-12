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

#include "Console.h"
#include "Options.h"
#include <cwchar>
#include <csignal>
#include <cstdarg>
#include <cstdlib>
#include <cstdio>
#include <unistd.h>
#include <termios.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>

const char ESC = 0x1b;

const uint8_t ATTR_OFF = 0;
const uint8_t ATTR_BOLD = 1;
const uint8_t ATTR_UNDERSCORE = 4;
const uint8_t ATTR_BLINK = 5;
const uint8_t ATTR_INVERSE = 7;
const uint8_t ATTR_CONCEALED = 8;

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

static long millis() {
    timeval time;
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

void Console::cleanup() {
    if (!this->aborting) {
        this->reset();
        this->clear();
        this->show_cursor(true);
        tcsetattr(0, TCSANOW, &initial_termios);
    }
    this->aborting = true;
}

Console::Console() {
    this->aborting = false;
    this->resize();
    this->reset();
    this->clear();
    // install sigwinch handler (window resize signal)
    signal(SIGWINCH, resize_handler);
    signal(SIGINT, exit_handler);
    signal(SIGTERM, exit_handler);

    tcgetattr(0, &initial_termios);
}

Console::~Console() {
//    printf("destruct Console\n");
    this->cleanup();
}

void Console::abort(const char *fmt, ...) {
    va_list ap;

    va_start(ap, fmt);
    this->cleanup();
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fflush(stdout);
}

void Console::resize() {
    winsize size{0};
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &size);
    this->width = size.ws_col;
    this->height = size.ws_row;
    this->clear();
}

void Console::raw(bool on) {
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
    this->raw_input = on;
}

bool Console::getch(int *c, bool timeout) {
    char cc = '\0';
    if (timeout) {
        long when = millis() + options.read_timeout;
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
        return false;
    }
    if (read(0, &cc, 1) == 1) {
        *c = int(cc);
        return true;
    }
    return false;
}

void Console::show_cursor(bool on) {
    if (on) {
        fputs("\e[?25h", stdout);
        cursor_hidden = false;
    } else {
        fputs("\e[?25l", stdout);
        cursor_hidden = true;
    }
    fflush(stdout);
}

/** @public **/
void Console::clear(bool endOfScreen) {
    printf("%c[%dJ", ESC, endOfScreen ? 0 : 2);
    fflush(stdout);
    this->moveTo(0, 0);
}

/** @public **/
void Console::reset() {
    this->set_mode(ATTR_OFF, true);
    this->background = this->foreground = 0;
    this->show_cursor(true);
}

/** @public **/
void Console::clear_eol() {
    printf("%c[0K", ESC);
    fflush(stdout);
}

/** @public **/
void Console::moveTo(uint16_t row, uint16_t col) {
    printf("%c[%d;%dH", ESC, row, col);
    fflush(stdout);
    this->row = row;
    this->col = col;
}

/** @private */
void Console::set_mode(uint8_t attr, bool on) {
    if (on) {
        printf("%c[%dm", ESC, attr);
    } else {
        printf("%c[=%dl", ESC, attr);
    }
    fflush(stdout);
    switch (attr) {
        case ATTR_OFF:
            this->bold = this->underscore = this->blink = this->inverse =
            this->concealed = false;
            this->show_cursor(!this->cursor_hidden);
            break;
        case ATTR_BOLD:
            this->bold = on;
            break;
        case ATTR_UNDERSCORE:
            this->underscore = on;
            break;
        case ATTR_BLINK:
            this->blink = on;
            break;
        case ATTR_INVERSE:
            this->inverse = on;
            break;
        case ATTR_CONCEALED:
            this->concealed = on;
            break;
        default:
            break;
    }
}

void Console::mode_clear() {
    this->set_mode(ATTR_OFF, true);
    fflush(stdout);
}

/** @public **/
void Console::mode_bold(bool on) { this->set_mode(ATTR_BOLD, on); }

/** @public **/
void Console::mode_underscore(bool on) { this->set_mode(ATTR_UNDERSCORE, on); }

/** @public **/
void Console::mode_blink(bool on) { this->set_mode(ATTR_BLINK, on); }

/** @public **/
void Console::mode_inverse(bool on) { this->set_mode(ATTR_INVERSE, on); }

/** @public **/
void Console::mode_concealed(bool on) { this->set_mode(ATTR_CONCEALED, on); }

/** @private */
void Console::set_color(uint8_t color, bool on) {
    if (on) {
        printf("%c[%dm", ESC, color);
    } else {
        printf("%c[%dl", ESC, color);
    }
    fflush(stdout);
}

/** @public **/
void Console::colors_clear() {
    if (this->background) {
        this->set_color(background, false);
    }
    if (this->foreground) {
        this->set_mode(foreground, false);
    }
    this->background = this->foreground = 0;
}

/** @public **/
void Console::bg_clear() {
    if (this->background) {
        this->set_mode(background, false);
    }
    this->background = 0;
}

/** @public **/
void Console::fg_clear() {
    if (this->foreground) {
        this->set_mode(foreground, false);
    }
    this->foreground = 0;
}

/** @public **/
void Console::fg_rgb(uint8_t red, uint8_t green, uint8_t blue) {
    printf("\e[38;2;%d;%d;%d;m", red, green, blue);
}

/** @public **/
void Console::bg_rgb(uint8_t red, uint8_t green, uint8_t blue) {
    printf("\e[48;2;%d;%d;%d;m", red, green, blue);
}

/** @public **/
void Console::bg_black() {
    this->set_color(BG_BLACK, true);
    this->background = BG_BLACK;
}

/** @public **/
void Console::fg_black() {
    this->set_color(FG_BLACK, true);
    this->foreground = FG_BLACK;
}

/** @public **/
void Console::bg_red() {
    this->set_color(BG_RED, true);
    this->background = BG_RED;
}

/** @public **/
void Console::fg_red() {
    this->set_color(FG_RED, true);
    this->foreground = FG_RED;
}

/** @public **/
void Console::bg_green() {
    this->set_color(BG_GREEN, true);
    this->background = BG_GREEN;
}

/** @public **/
void Console::fg_green() {
    this->set_color(FG_GREEN, true);
    this->foreground = FG_GREEN;
}

/** @public **/
void Console::bg_yellow() {
    this->set_color(BG_YELLOW, true);
    this->background = BG_YELLOW;
}

/** @public **/
void Console::fg_yellow() {
    this->set_color(FG_YELLOW, true);
    this->foreground = FG_YELLOW;
}

/** @public **/
void Console::bg_blue() {
    this->set_color(BG_BLUE, true);
    this->background = BG_BLUE;
}

/** @public **/
void Console::fg_blue() {
    this->set_color(FG_BLUE, true);
    this->foreground = FG_BLUE;
}

/** @public **/
void Console::bg_magenta() {
    this->set_color(BG_MAGENTA, true);
    this->background = BG_MAGENTA;
}

/** @public **/
void Console::fg_magenta() {
    this->set_color(FG_MAGENTA, true);
    this->foreground = FG_MAGENTA;
}

/** @public **/
void Console::bg_cyan() {
    this->set_color(BG_CYAN, true);
    this->background = BG_CYAN;
}

/** @public **/
void Console::fg_cyan() {
    this->set_color(FG_CYAN, true);
    this->foreground = FG_CYAN;
}

/** @public **/
void Console::bg_white() {
    this->set_color(BG_WHITE, true);
    this->background = BG_WHITE;
}

/** @public **/
void Console::fg_white() {
    this->set_color(FG_WHITE, true);
    this->foreground = FG_WHITE;
}

void Console::print(const char *fmt, ...) {
    va_list ap;

    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);
    fflush(stdout);
}

void Console::println(const char *fmt, ...) {
    va_list ap;

    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);
    fflush(stdout);
    this->newline();
}

void Console::inverseln(const char *fmt, ...) {
    va_list ap;

    va_start(ap, fmt);
    console.bg_white();
    console.fg_black();
    vprintf(fmt, ap);
    console.clear_eol();
    va_end(ap);
    fputs("\n", stdout);
    console.mode_clear();
    fflush(stdout);
}

void Console::wprint(const wchar_t *fmt, ...) {
    va_list ap;

    va_start(ap, fmt);
    vwprintf(fmt, ap);
    va_end(ap);
    fflush(stdout);
}

void Console::wprintln(const wchar_t *fmt, ...) {
    va_list ap;

    va_start(ap, fmt);
    vwprintf(fmt, ap);
    va_end(ap);
    this->clear_eol();
    fputs("\n", stdout);
    fflush(stdout);
}

void Console::gauge(int aWidth, double pct, char fill) {
    this->print("[");
    int toFill = aWidth * pct / 100.,
            i;
    for (i = 0; i < toFill; i++) {
        this->print("%c", fill);
    }
    while (i < aWidth) {
        this->print(" ");
        i++;
    }
    this->print("]");
}

void Console::window(int aRow, int aCol, int aWidth, int aHeight, const char *aTitle) {
    int w = aWidth - 2;
    int h = aHeight - 2;
    int r = aRow;

    // top row
    moveTo(r++, aCol);
    wprint(L"%lc", 0x2554);
    for (int i = 0; i < w; i++) {
        wprint(L"%lc", 0x2550);
    }
    wprint(L"%lc", 0x2557);

    // middle rows
    for (int hh = 0; hh < h; hh++) {
        moveTo(r++, aCol);
        wprint(L"%lc", 0x2551);
        for (int i = 0; i < w; i++) {
            wprint(L"%lc", ' ');
        }
        wprint(L"%lc", 0x2551);
    }
    // bottom row
    moveTo(r++, aCol);
    wprint(L"%lc", 0x255a);
    for (int i = 0; i < w; i++) {
        wprint(L"%lc", 0x2550);
    }
    wprint(L"%lc", 0x255d);

    if (aTitle) {
        moveTo(aRow, aCol + 2);
        print("[%s]", aTitle);
    }
}

void Console::newline(bool erase) {
    if (erase) {
        this->clear_eol();
    }
    fputs("\n", stdout);
    fflush(stdout);
}

Console console;
