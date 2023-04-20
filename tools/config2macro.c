#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <common/escp.h>

#define BUFFER_SIZE 4096

#define CFG(key, value) const char* key = value;

/*
 *  The tool `config2macro` is used to read configuration files (THING = VALUE) and convert them to preprocessor macros
 *  like `#define THING VALUE`...
 *  This will be used for getting our C code to be able to read configuration...
 */

int main(int argc, char* argv[]) {
    if (argc != 2) {
        printf("%susage: %sconfig2macro %s<%sconfig_file%s>%s\n", F_MAGENTA, F_GREEN, F_BLUE, F_GREEN, F_BLUE, RESET);
        return 1;
    }

    FILE* file = fopen(argv[1], "r");
    if (file == NULL) {
        printf("error: cannot open file '%s'\n", argv[1]);
        return 1;
    }

    char line[BUFFER_SIZE];
    while (fgets(line, sizeof(line), file)) {
        char* key = strtok(line, "=\n");
        char* value = strtok(NULL, "=\n");
        if (key != NULL && value != NULL) {
            key = strtok(key, " \t");
            value = strtok(value, " \t");

            /* Generate preprocessor `#define`s... */
            char macro[BUFFER_SIZE];
            snprintf(macro, sizeof(macro), "#define %s \"%s\"\n", key, value);
            printf("%s", macro);
        }
    }

    fclose(file);
    return 0;
}
