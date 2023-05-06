#define _XOPEN_SOURCE 500
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>

#define MAX_DIR_LENGTH 101
#define MAX_DIR_COUNT 30

#define ERR(source) (perror(source), \
fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), \
exit(EXIT_FAILURE))

void usage(const char *path)
{
    fprintf(stderr, "usage: %s", path);
    exit(EXIT_FAILURE);
}

void listdir(FILE* file, const char* dir, const char* cwd)
{
    DIR* dirp;
    struct dirent *dp;
    struct stat sb;

    if (-1 == chdir(dir))
        ERR("Unable to change directory");

    if ((dirp = opendir(".")) == NULL) 
        ERR("Couldn't open directory");

    do {
        errno = 0;

        if ((dp = readdir(dirp)) != NULL) 
        {
            if (lstat(dp->d_name, &sb) == -1)
            {
                ERR("The lstat() function failed");
            }
            fprintf(file, "%s %ld\n", dp->d_name, sb.st_size);
        }
    } while (dp != NULL);

    if (errno != 0)
        ERR("The readdir() function failed");

    closedir(dirp);

    if (-1 == chdir(cwd))
        ERR("Unable to change directory");
}

int main (int argc, char **argv)
{ 
    int option;
    char* file_name = NULL;
    int dirscount = 0;
    char cwd[MAX_DIR_LENGTH];
    char* dirs[MAX_DIR_COUNT];

    getcwd(cwd, MAX_DIR_LENGTH);

    if (cwd == NULL)
        ERR("Unable to read current working directory");

    while((option = getopt(argc, argv, ":o:p:")) != -1)
    {
        switch (option)
        {
            case 'o':
                if (file_name != NULL)
                {
                    usage(argv[0]);
                }
                file_name = optarg;
            break;
            case 'p':
                dirs[dirscount++] = optarg;
            break;
            case '?':
            case ':':
                usage(argv[0]);
            default:
            break;
        }
    }

    FILE* outstream = stderr;

    if (NULL != file_name)
        outstream = fopen(file_name, "w");

    if (NULL == outstream)
        ERR("Opening file error");

    if (dirscount == 0)
        listdir(outstream, ".", cwd);

    for (int i = 0; i < dirscount; i++)
        listdir(outstream, dirs[i], cwd);

    if (outstream != stderr)
        fclose(outstream);

    return EXIT_SUCCESS;
}