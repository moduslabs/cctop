//
// Created by Michael Schwartz on 12/11/21.
//

#include "Docker.h"
#include "../lib/Console.h"
#include <curl/curl.h>

static size_t write_data(void *ptr, size_t size, size_t nmemb, void *stream) {
    size_t written = fwrite(ptr, size, nmemb, (FILE *) stream);
    return written;
}

int curl_test() {
    curl_global_init(CURL_GLOBAL_ALL);

    CURL *curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_URL, "https://www.google.com");

    FILE *outfile = fopen("/tmp/foo.txt", "w");
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, outfile);

    CURLcode res;
    res = curl_easy_perform(curl);
    fprintf(outfile, "\n\n\n");
    fclose(outfile);
    return 0;
    curl_easy_cleanup(curl);
    curl_global_cleanup();
    console.bg_white();
    console.fg_black();
    console.mode_blink();

}

