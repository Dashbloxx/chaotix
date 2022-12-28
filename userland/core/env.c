#include <stdio.h>
#include <stdlib.h>

int main(int argc, char* const argv[], char* const envp[]) {
    (void)argc;
    (void)argv;
    while (*envp)
        puts(*envp++);
    return EXIT_SUCCESS;
}
