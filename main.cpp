#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>
#include <unistd.h>
#include <stdarg.h>

int wprintf(const wchar_t *format, ...);

#include "lib/Console.h"
#include <curl/curl.h>
#include <locale.h>

#include "macos/Platform.h"
#include "macos/Processor.h"
#include "macos/Memory.h"
#include "macos/Disk.h"
#include "macos/Network.h"
#include "macos/ProcessList.h"
#include "macos/Battery.h"

static size_t write_data(void *ptr, size_t size, size_t nmemb, void *stream) {
    size_t written = fwrite(ptr, size, nmemb, (FILE *) stream);
    return written;
}

#pragma clang diagnostic push
#pragma ide diagnostic ignored "EndlessLoop"
int main() {
    setlocale(LC_ALL, "");

//    curl_global_init(CURL_GLOBAL_ALL);

//    CURL *curl = curl_easy_init();
//    curl_easy_setopt(curl, CURLOPT_URL, "https://www.google.com");

//    FILE *outfile = fopen("/tmp/foo.txt", "w");
//    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
//    curl_easy_setopt(curl, CURLOPT_WRITEDATA, outfile);

//    CURLcode res;
//    res = curl_easy_perform(curl);
//    fprintf(outfile, "\n\n\n");
//    fclose(outfile);
//    return 0;
//    curl_easy_cleanup(curl);
//    curl_global_cleanup();
//    console.bg_white();
//    console.fg_black();
//    console.mode_blink();
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
    for (;;) {
        console.moveTo(1, 1);

        platform.update();
        platform.print(false);
        console.newline();

        processor.update();
        processor.print(false);
        console.newline();

        memory.update();
        memory.print(false);
        console.newline();

        disk.update();
        disk.print(false);
        console.newline();

        network.update();
        network.print(false);
//        wprintf(L"%p %lc\n", b, b);
        processList.update();
        processList.print(false);
//        wprintf(L"%p %lc\n", b, b);
        sleep(1);
    }
#endif
    return 0;
}
#pragma clang diagnostic pop
