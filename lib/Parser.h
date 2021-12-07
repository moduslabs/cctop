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
#ifndef PARSER_H
#define PARSER_H

#include <fcntl.h>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>

struct Line {
  char *line, *token;
  size_t len;

public:
  explicit Line(char *l);
  ~Line();

public:
  const char *get_token();
};

class Parser {
protected:
  FILE *fp;
  const char *filename;
  Line *line;
  const char *token;

public:
  Parser(const char *filename);
  ~Parser();

public:
  // next line, returns false if EOF
  bool next();
  const char *get_token();
  uint64_t get_long();
};

#endif
