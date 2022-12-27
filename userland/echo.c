#include <stdio.h>
#include <stdlib.h>

int main(int argc, char* const argv[]) {
    if (argc < 2) {
        putchar('\n');
        return EXIT_SUCCESS;
    }
    for (int i = 1; i < argc - 1; ++i)
        printf("%s ", argv[i]);
    puts(argv[argc - 1]);
    return EXIT_SUCCESS;
}
