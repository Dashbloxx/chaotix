#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main(void) {
    time_t now;
    if (time(&now) < 0) {
        perror("time");
        return EXIT_FAILURE;
    }
    struct tm tm;
    char buf[1024];
    asctime_r(gmtime_r(&now, &tm), buf);
    puts(buf);
    return EXIT_SUCCESS;
}
