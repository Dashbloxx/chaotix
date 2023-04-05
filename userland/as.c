#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <escp.h>

/* Basic list of architectures... */
enum {
    i386,
    amd64,
    aarch64
};

typedef unsigned char arch_t;

int parse_asm(arch_t architecture, char *assembly, char *binary) {
    if (architecture == i386) {
        if (strcmp(assembly, "mov") == 0) {
            strcat(binary, "B8 ");
        }
        else if (strcmp(assembly, "add") == 0) {
            strcat(binary, "03 ");
        }
        else if (strcmp(assembly, "sub") == 0) {
            strcat(binary, "2B ");
        }
        else if (strcmp(assembly, "cmp") == 0) {
            strcat(binary, "3B ");
        }
        else if (strcmp(assembly, "jmp") == 0) {
            strcat(binary, "E9 ");
        }
        else if (strcmp(assembly, "call") == 0) {
            strcat(binary, "E8 ");
        }
        else if (strcmp(assembly, "ret") == 0) {
            strcat(binary, "C3 ");
        }
        else if (strcmp(assembly, "push") == 0) {
            strcat(binary, "50 ");
        }
        else if (strcmp(assembly, "pop") == 0) {
            strcat(binary, "58 ");
        }
    }
    else if (architecture == amd64) {
        /* Not supported yet! */
        return -1;
    }
    else if (architecture == aarch64) {
        /* Not supported yet! */
        return -1;
    }
    else {
        /* Unknown architecture... */
        return -2;
    }
    return 0;
}

int main(int argc, const char *argv[]) {
    if (argc <= 2) {
        /* See line 91-94 regarding the reason why "\x1b[37m" is used instead of F_WHITE... */
        printf("%serror: %snot enough arguments specified%s\n", F_RED, "\x1b[37m", RESET);
        /* Let's also print out the usage of the `as` command. Later on, we can add in a manual for it aswell... */
        printf("%susage: %sas %s<%sarchitecture%s> <%sinstruction(s)%s>%s\n", F_MAGENTA, F_GREEN, F_BLUE, F_GREEN, F_BLUE, F_GREEN, F_BLUE, RESET);
        return 1;
    }

    char assembly[1024] = "";

    /*
     *  Paste all arguments into `assembly` string. This allows us to call the `as` command like this:
     *      $ as <instructions>
     *  Instead of this:
     *      $ as '<instructions>'
     */
    for (int i = 2; i < argc; i++) {
        strcat(assembly, argv[i]);
        if (i < argc - 1) {
            strcat(assembly, " ");
        }
    }

    char binary[1024] = ""; // Initialize binary string with empty string
    for (int i = 0; i < strlen(assembly); i += 4) {
        char instruction[5];
        strncpy(instruction, assembly + i, 3);
        instruction[3] = '\0';
        if (strcmp(argv[1], "i386") == 0) {
            parse_asm(i386, instruction, binary);
        }
        else {
            /*
             *  For some very, very mysterious reason, the compiler says that F_WHITE is undeclared, although `escp.h` is included, and
             *  contains the definition of F_WHITE...
             */
            printf("%serror: %sunknown or unsupported architecture...%s\n", F_RED, "\x1b[37m", RESET);
            return -3;
        }
    }
    printf("%s\n", binary);
    return 0;
}