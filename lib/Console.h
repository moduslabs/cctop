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

// GENERAL PURPOSE CONSOLE
// use ANSI escape sequences to control output to the console/terminal window.

#ifndef C_CONSOLE_H
#define C_CONSOLE_H

#define _XOPEN_SOURCE_EXTENDED
#include <cstdint>
#include <cstdio>
#include <sys/ioctl.h>
#include <termios.h>

class Console {
public:
    // console window width and height
    uint16_t width{}, height{};
    struct termios initial_termios{0};

private:
    bool aborting{}, pad{};
    int current_pair{-1};

    uint16_t row{}, col{}; // cursor location

    // modes
    bool bold{false},
            underscore{false},
            blink{false},
            inverse{false},
            concealed{false},
            cursor_hidden{false},
            raw_input{false};

    // colors
    uint8_t background{}, foreground{};

public:
    Console();

    ~Console();

public:
    // print a message and exit cleanly with exit code 1
    void abort(const char *fmt, ...);

    void cleanup();

public:
    // Call to update Console's notion of width and height.
    // Also clears the screen/window.
    void resize();
    void update();

public:
    void raw(bool on = true);

    bool read_character(int *c, bool timeout = false);

public:
    // enable/disable cursor
    void show_cursor(bool on = true);

    // clear screen
    void clear(bool endOfScreen = false);

    // reset all modes and colors, enable cursor
    void reset();

    // clear to end of line
    void clear_eol();

    // address cursor
    void moveTo(uint16_t r, uint16_t c);

    // printf style output to terminal
    void print(const char *fmt, ...);

    // printf style output to terminal, with newline
    void println(const char *fmt, ...);
    // print line inverse, with newline

    void inverseln(const char *fmt, ...);

    // wprintf style output to terminal
    void wprintf(const wchar_t *fmt, ...);

    // printf style output to terminal, with newline
    void wprintfln(const wchar_t *fmt, ...);
    // print line inverse, with newline

    // emit a newline
    void newline(bool erase = true);

    // print a gauge, representing the specified percentage
    void gauge(int width, double pct, char fill = '>');

    void window(int aRow, int aCol, int aWidth, int aHeight, const char *title = "Untitled");

private:
    void set_mode(uint8_t attr, bool on);

public:
    // reset any modes that are set.
    void mode_clear();

    // turn on/off bold
    void mode_bold(bool on = true);

    // turn on/off underscore
    void mode_underscore(bool on = true);

    // turn on/off blink
    void mode_blink(bool on = true);

    // turn on/off inverse
    void mode_inverse(bool on = true);

public:
    void default_colors();
    void set_colors(uint8_t fg, uint8_t bg);
    void set_foreground(uint8_t color);
    void set_background(uint8_t color);

public:
    // reset foreground/background colors to default
    void colors_clear();

    // set true color foreground color
    void fg_rgb(uint8_t red, uint8_t green, uint8_t blue);

    // set true color background color
    void bg_rgb(uint8_t red, uint8_t green, uint8_t blue);

    // reset foreground color to default
    void fg_clear();

    // use black as foreground color
    void fg_black();

    // use red as foreground color
    void fg_red();

    // use green as foreground color
    void fg_green();

    // use yellow as foreground color
    void fg_yellow();

    // use blue as foreground color
    void fg_blue();

    // use magenta as foreground color
    void fg_magenta();

    // use cyan as foreground color
    void fg_cyan();

    // use white as foreground color
    void fg_white();

    // reset foreground color to default
    void bg_clear();

    // use black as background color
    void bg_black();

    // use red as background color
    void bg_red();

    // use green as background color
    void bg_green();

    // use yellow as background color
    void bg_yellow();

    // use blue as background color
    void bg_blue();

    // use magenta as background color
    void bg_magenta();

    // use cyan as background color
    void bg_cyan();

    // use white as background color
    void bg_white();
};

extern Console console;

#endif //CCTOP_CONSOLE_H
