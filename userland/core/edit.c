#include <stdio.h>
#include <stdlib.h>
#include <kernel/api/fcntl.h>

int main(int argc, char * argv[])
{
    if(argc == 1)
    {
        // Looks like we've just gotten the command called, and no arguments!
        printf("\x1b[37;1medit: \x1b[31;1mfatal error: \x1b[0myou haven't specified a file to edit and/or create!\n");
    } else if(argc == 2) {
        // Let's check if the filename passed is a real file...
        int fd0 = open(argv[1], O_CREAT, 0);
        if (fd0 < 0) { }
        close(fd0);

        int fd1 = open(argv[1], O_RDWR);
        if(fd1 < 0)
        {
            printf("\x1b[37;1medit: \x1b[31;1mfatal error: \x1b[0mthere was an error opening the file...\n");
        } else
        {
            printf("\x1b[37;1medit: \x1b[31;1mfatal error: \x1b[0mfile editor is work-in-progress and is not finished yet!\n")
        }
    }
    return EXIT_SUCCESS;
}