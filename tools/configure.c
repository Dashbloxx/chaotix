#include <stdio.h>
#include <common/escp.h>
#include <string.h>

/* Useful for setting the value of a key in a configuration file... */
int set_key_value(const char *filename, const char *key, const char *value) {
    char line[4096];
    FILE *fp;
    long int pos;
    int found_key = 0;

    /* Open file as `r+`, because we're going to be reading from the configuration file and writing into it... */
    fp = fopen(filename, "r+");
    if (fp == NULL) {
        printf("error: unable to open file %s\n", filename);
        return 1;
    }

    /* Find the key, and set it's value... */
    while (fgets(line, sizeof(line), fp) != NULL) {
        /* Check if line contains the key we want to set */
        if (strstr(line, key) == line) {
            found_key = 1;
            pos = ftell(fp);
            fseek(fp, pos - strlen(line), SEEK_SET);
            fprintf(fp, "%s = %s\n", key, value);
            fseek(fp, pos - 1, SEEK_SET);
            while (fgetc(fp) != '\n' && !feof(fp)) { }
            break;
        }
    }

    /* Check if the key was found... */
    if (!found_key) {
        printf("error: key %s not found in file\n", key);
        fclose(fp);
        return 1;
    }

    fclose(fp);

    return 0;
}

int main(int argc, char *argv[]) {
    char *arch = NULL;
    char *bootloader = NULL;
    char *model = NULL;
    char *font = NULL;
    
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--arch") == 0 && i+1 < argc) {
            arch = argv[i+1];
        } else if (strcmp(argv[i], "--bootloader") == 0 && i+1 < argc) {
            bootloader = argv[i+1];
        } else if (strcmp(argv[i], "--model") == 0 && i+1 < argc) {
            model = argv[i+1];
        } else if (strcmp(argv[i], "--font") == 0 && i+1 < argc) {
            font = argv[i+1];
        }
    }
    
    if (arch != NULL) {
        printf("info: value after --arch is: %s\n", arch);
        set_key_value(argv[1], "ARCHITECTURE", arch);
    }
    
    if (bootloader != NULL) {
        printf("info: value after --bootloader is: %s\n", bootloader);
        set_key_value(argv[1], "BOOTLOADER", bootloader);
    }
    
    if (model != NULL) {
        printf("info: value after --model is: %s\n", model);
        set_key_value(argv[1], "FORM", model);
    }

    if (font != NULL) {
        printf("info: value after --font is: %s\n", font);
        set_key_value(argv[1], "FONT", font);
    }
    
    return 0;
}
