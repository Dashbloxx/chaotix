#include <stdio.h>
#include <stdlib.h>
#include <sys/reboot.h>
#include <unistd.h>

int main(void) {
    if (reboot(RB_POWEROFF) < 0)
        perror("reboot");
    return EXIT_FAILURE;
}
