#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <escp.h>

/*
 *  This is a basic assembler written in C. The plan for magma is for it to have compatibility, but to also have it's own custom toolchain
 *  where in the future is capable of building itself. For now, the assembler is being developed.
 *  For now though, all the assembler does is replace instructions with their opcodes.
 *  Here is an example of the assembler's usage:
 *      $ as i386 <instruction(s)>
 */

/* Basic list of architectures... */
enum {
    i386,
    amd64,
    aarch64
};

/* Basic type that represents different architectures, using the enum above... */
typedef unsigned char arch_t;

/*
 *  This function accepts four arguments; the first one asking which architecture to assemble for, the second one containing the
 *  options (3 characters), the third containing the assembly, and the fourth containing a pointer to a buffer where the binary
 *  output code is to be stored...
 * 
 *  If the `options` pointed value is `str`, it will just convert the assembly to opcodes, but as a string. If the `options` pointed
 *  value is `bin`, it will convert the assembly to actual raw binary...
 */
int parse_asm(arch_t architecture, char options[3], char *assembly, void *binary) {
    if (architecture == i386) {
        /* Handle i386 instructions... */
        if (strcmp(assembly, "mov") == 0) {
            if(options == "str") {
                strcat(binary, "0xB8 ");
            }
            else if(options == "bin") {
                strcat(binary, 0xb8);
            }
        }
        else if (strcmp(assembly, "add") == 0) {
            if(options == "str") {
                strcat(binary, "0x03 ");
            }
            else if(options == "bin") {
                strcat(binary, 0x03);
            }
        }
        else if (strcmp(assembly, "sub") == 0) {
            if(options == "str") {
                strcat(binary, "0x2B ");
            }
            else if(options == "bin") {
                strcat(binary, 0x2b);
            }
        }
        else if (strcmp(assembly, "cmp") == 0) {
            if(options == "str") {
                strcat(binary, "0x3B ");
            }
            else if(options == "bin") {
                strcat(binary, 0x3b);
            }
        }
        else if (strcmp(assembly, "jmp") == 0) {
            if(options == "str") {
                strcat(binary, "0xE9 ");
            }
            else if(options == "bin") {
                strcat(binary, 0xe9);
            }
        }
        else if (strcmp(assembly, "call") == 0) {
            if(options == "str") {
                strcat(binary, "0xE8 ");
            }
            else if(options == "bin") {
                strcat(binary, 0xe8);
            }
        }
        else if (strcmp(assembly, "ret") == 0) {
            if(options == "str") {
                strcat(binary, "0xC3 ");
            }
            else if(options == "bin") {
                strcat(binary, 0xc3);
            }
        }
        else if (strcmp(assembly, "push") == 0) {
            if(options == "str") {
                strcat(binary, "0x50 ");
            }
            else if(options == "bin") {
                strcat(binary, 0x50);
            }
        }
        else if (strcmp(assembly, "pop") == 0) {
            if(options == "str") {
                strcat(binary, "0x58 ");
            }
            else if(options == "bin") {
                strcat(binary, 0x58);
            }
        }
        /* Handle i386 registers... */
        else if (strcmp(assembly, "eax") == 0) {
            if(options == "str") {
                strcat(binary, "0x00 ");
            }
            else if(options == "bin") {
                strcat(binary, 0x00);
            }
        }
        else if (strcmp(assembly, "ebx") == 0) {
            if(options == "str") {
                strcat(binary, "0x03 ");
            }
            else if(options == "bin") {
                strcat(binary, 0x03);
            }
        }
        else if (strcmp(assembly, "ecx") == 0) {
            if(options == "str") {
                strcat(binary, "0x01 ");
            }
            else if(options == "bin") {
                strcat(binary, 0x01);
            }
        }
        else if (strcmp(assembly, "edx") == 0) {
            if(options == "str") {
                strcat(binary, "0x02 ");
            }
            else if(options == "bin") {
                strcat(binary, 0x02);
            }
        }
        else if (strcmp(assembly, "esi") == 0) {
            if(options == "str") {
                strcat(binary, "0x06 ");
            }
            else if(options == "bin") {
                strcat(binary, 0x06);
            }
        }
        else if (strcmp(assembly, "edi") == 0) {
            if(options == "str") {
                strcat(binary, "0x07 ");
            }
            else if(options == "bin") {
                strcat(binary, 0x07);
            }
        }
        else if (strcmp(assembly, "ebp") == 0) {
            if(options == "str") {
                strcat(binary, "0x05 ");
            }
            else if(options == "bin") {
                strcat(binary, 0x05);
            }
        }
        else if (strcmp(assembly, "esp") == 0) {
            if(options == "str") {
                strcat(binary, "0x04 ");
            }
            else if(options == "bin") {
                strcat(binary, 0x04);
            }
        }
        /* Handle unknown object... */
        else {
            return -3;
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
            parse_asm(i386, "str", instruction, binary);
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