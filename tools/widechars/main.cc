#include <cstdio>
#include <wchar.h>
#include <clocale>

int main(int ac, char *av[]) {
    setlocale(LC_ALL, "");
  for (wchar_t c = 0x2000; c<0x3000; c++) {
    wprintf(L"%l04x %lc \n", c, c);
  }

  wprintf(L"2581 %lc \n", 0x2581);

  return 1;
}

