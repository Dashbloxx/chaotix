#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int parse_asm(char *assembly, char *binary) {
    if (strcmp(assembly, "mov") == 0) {
        strcat(binary, "B8 ");
    }
    else if (strcmp(assembly, "add") == 0) {
        strcat(binary, "83C001 ");
    }
    return 0;
}


int main(int argc, const char *argv[]) {
    char assembly[1024] = "";

    /*
     *  Paste all arguments into `assembly` string. This allows us to call the `as` command like this:
     *      $ as <instructions>
     *  Instead of this:
     *      $ as '<instructions>'
     */
    for (int i = 1; i < argc; i++) {
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
        parse_asm(instruction, binary);
    }
    printf("%s\n", binary);
    return 0;
}