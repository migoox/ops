#define _XOPEN_SOURCE 500
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <string.h>
#include <ftw.h>
#include <sys/stat.h>

#define MAX_DIR_LENGTH 101
#define MAX_DIR_COUNT 30
#define DEFAULT_DEPTH 1
#define MAXFD 20

#define ERR(source) (perror(source), \
fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), \
exit(EXIT_FAILURE))

void usage(const char *path)
{
    fprintf(stderr, "usage: %s\n", path);
    exit(EXIT_FAILURE);
}

FILE* outstream;
char* extension = NULL;
int depth = DEFAULT_DEPTH;

static int step(const char *fpath, const struct stat *statbuf,
               int tflag, struct FTW *ftwbuf)
{
    if (ftwbuf->level > depth)
        return 0;

    char *curr_extension;
    if (extension != NULL)
    {
        if (NULL == (curr_extension = strrchr(fpath, '.')))
        {
            return 0;
        }
        if (strcmp(++curr_extension, extension) != 0)
        {
            return 0;
        }
    }

    char *name;
    if (NULL == (name = strrchr(fpath, '/')))
        fprintf(outstream, "%s %ld\n", fpath, statbuf->st_size);
    else
        fprintf(outstream, "%s %ld\n", ++name, statbuf->st_size);

    return 0;
}

int main (int argc, char **argv)
{ 
    int option;
    int use_environ = 0;
    int dirscount = 0;
    char cwd[MAX_DIR_LENGTH];
    char* dirs[MAX_DIR_COUNT];

    getcwd(cwd, MAX_DIR_LENGTH);

    if (cwd == NULL)
        ERR("Unable to read current working directory");

    while((option = getopt(argc, argv, ":d:p:oe:")) != -1)
    {
        switch (option)
        {
            case 'o':
                use_environ = 1;
            break;
            case 'p':
                dirs[dirscount++] = optarg;
            break;
            case 'd':
                errno = 0;
                char *endptr = NULL;
                depth = strtoul(optarg, &endptr, 8);
                if (errno != 0 || endptr == optarg || *endptr != '\0')
                    ERR("The strtol() function error");

                if(depth < 0)
                    usage(argv[0]);
            break;
            case 'e':
                extension = optarg;
            break;
            case '?':
            case ':':
                usage(argv[0]);
            default:
            break;
        }
    }

    char* file_name = NULL;
    outstream = stderr;

    if (use_environ == 1 && NULL != (file_name = getenv("L1_OUTPUTFILE")))
    {
       outstream = fopen(file_name, "w+");
    }

    if (NULL == outstream)
        ERR("Opening file error");

    if (dirscount == 0)
        nftw(".", step, MAXFD, FTW_PHYS);

    for (int i = 0; i < dirscount; i++)
    {
        nftw(dirs[i], step, MAXFD, FTW_PHYS);
    }
    if (outstream != stderr)
        fclose(outstream);

    return EXIT_SUCCESS;
}