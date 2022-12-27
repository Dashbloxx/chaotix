#include "stdlib.h"
#include "unistd.h"

int main(int argc, char* const argv[], char* const envp[]);

void _start(int argc, char* const argv[], char* const envp[]) {
    environ = (char**)envp;
    exit(main(argc, argv, envp));
}
