#include <stdio.h>
#include <stdlib.h>

int main(void) {
    printf("\x1b[30;47m ______                 _     _       \n"
    "(_____ \\               | |   (_)      \n"
    " _____) )__ _   _  ____| |__  _ _   _ \n"
    "|  ____/___) | | |/ ___)  _ \\| ( \\ / )\n"
    "| |   |___ | |_| ( (___| | | | |) X ( \n"
    "|_|   (___/ \\__  |\\____)_| |_|_(_/ \\_)\n"
    "           (____/                     \n\x1b[0m");
    printf("Psychx is a re-implementation of the UNIX operating system. It is written in C, and borrows code from yagura & Xv6. It is licensed under the MIT license, and created and maintained by Nexuss and some other friends (\x1b[34;4mhttps://github.com/Dashbloxx/\x1b[0m). You can find the project at \x1b[34;4mhttps://github.com/Dashbloxx/\x1b[0m...\n");
    return EXIT_SUCCESS;
}
