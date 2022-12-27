#include <stdio.h>
#include <stdlib.h>

int main(void) {
    printf("\x1b[H\x1b[2J");
    return EXIT_SUCCESS;
}
