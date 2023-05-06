#define _XOPEN_SOURCE 500
#include <dirent.h>
#include <errno.h>
#include <ftw.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>
#define MAXFD 20

#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))

int main(int argc, char **argv)
{
    char option;
    const char* file_name = "file";
    mode_t permissions = 0777;
    unsigned int size = 1;

    // endptr required to strtol() function
    char* endptr;

    // reading options and theirs arguments
    while ((option = getopt(argc, argv, ":n:p:s:")) != -1)
    {
        switch (option)
        {
            case 'n':
                file_name = optarg;
                break;
            case 'p':
                errno = 0;
                endptr = NULL;

                permissions = strtoul(optarg, &endptr, 8);

                if (errno != 0 || endptr == optarg || *endptr != '\0')
                    ERR("The strtol() function error");
                break;  
            case 's':
                errno = 0;
                endptr = NULL;

                size = strtoul(optarg, &endptr, 10);

                if (errno != 0 || endptr == optarg || *endptr != '\0')
                    ERR("The strtol() function error");
                break;
            case ':':
                ERR("Missing option argument");
                break;
            case '?':
                ERR("Incorrect option");
                break;
            default:
                ERR("Uknown error");
            break;
        }
    }

    struct stat stat_buffer;

    // if file exists - delete it
    if (lstat(file_name, &stat_buffer) == 0)
    {
        // file exists
        if ((stat_buffer.st_mode & S_IFMT) == S_IFDIR)
        {
            // rmdir(file_name) function should delete directory
            ERR("A directory with the same name exists");
        }
        else
        {
            // without the first "if" we would use errno == ENOENT,
            // to indicate if the file exist or not 
            if (unlink(file_name) == -1)
                ERR("Cannot unlink the file");
        }

        // remove() works for directories and other files
    }

    // set current file mode creation mask,
    // the file mode creation maks of the process is used to 
    // turn off permission bits, so we need to use negation
    umask(~permissions & 0777);
    // note that changing mask is local for our process and that
    
    // fopen function doesn't allow us to set x permission bit

    // create a file and open it in "write" mode
    FILE* file = fopen(file_name, "w");
    if (file == NULL)
    {
        ERR("Cannot open create a file with the given name");
    }

    float percent = 0.1f;
    unsigned int count = (int)(percent * (float)size);

    // go to the last character and set it to NULL
    if (fseek(file, size - 1, SEEK_SET))
        ERR("fseek() error");

    fprintf(file, "%c", '\0');
    
    // fill randomly
    srand(time(NULL));

    for (unsigned int i = 0; i < count; i++)
    {
        if (fseek(file, rand() % size, SEEK_SET) == -1)
            ERR("fseek() error");

        fprintf(file, "%c", rand() % ('Z' - 'A' + 1) + 'A');
    }
    
    // close the file
    if (fclose(file) != 0)
    {
        ERR("Cannot close the file");
    }    

	return EXIT_SUCCESS;
}