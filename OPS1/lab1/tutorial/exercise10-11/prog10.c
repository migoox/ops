// without declaration of _XOPEN_SOURCE on Linux, the definition
// of nftw() is not seen 
#define _XOPEN_SOURCE 500
#include <dirent.h>
#include <errno.h>
#include <ftw.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
// max depth of the tree limit
#define DEFAULT_MAXFD 20

#define FILETYPES_COUNT 8

// option flags
static int be_verbose_flag = 0;
static int help_flag = 0;
static int recursive_flag = 0;
static int fd_limit = DEFAULT_MAXFD;

static const char *filetypes[] = 
{ 
    "block device", 
    "character device",
    "directory",
    "FIFO/pipe",
    "symlink",
    "regular file",
    "socket",
    "uknown?"
};

static const unsigned int filetypes_macros[] = 
{
    S_IFBLK,  
    S_IFCHR,  
    S_IFDIR,  
    S_IFIFO,  
    S_IFLNK,  
    S_IFREG,  
    S_IFSOCK,
};

static int filetypes_counts[FILETYPES_COUNT] = { 0 };

static void usage(const char* filename)
{
    fprintf(stderr, "USAGE: %s [-v] [-h] [PATHS]...\n", filename);
    exit(EXIT_FAILURE);
}

static void help(const char* filename)
{
    printf("Program \"%s\" allows counting filetypes.\n", filename);
    fprintf(stderr, "USAGE: %s [-v] [-h] [PATHS]...\n", filename);
    printf("Available optitons: \n");
    printf("  -v - be verbose\n");
    printf("  -h - help\n");
    printf("  -r - scan recursevily\n");
}

static int scanning_step(const char *fpath, const struct stat *statbuf,
               int tflag, struct FTW *ftwbuf)
{
    if (be_verbose_flag)
        printf("%s", fpath);
        
    // this is temp value which helps to determine
    // either the file type exists 
    int uknown = 1;
    
    // loop through the file types
    for (int i = 0; i < FILETYPES_COUNT - 1; i++)
    {
        if ((statbuf->st_mode & S_IFMT) == filetypes_macros[i])
        {
            if (be_verbose_flag)
                printf(" (%s)\n", filetypes[i]);
            uknown = 0;
            filetypes_counts[i]++;
            break;
        }
    }

    // if the if statement hadn't occured during the loop
    // mark the file as the uknown
    if (uknown == 1)
    {
        if (be_verbose_flag)
            printf(" (%s)\n", filetypes[FILETYPES_COUNT - 1]);
        filetypes_counts[FILETYPES_COUNT - 1]++;
    }

    return 0;
}

static void scan_dirpath(const char* dirpath, const char* abs_cwd, const char* program_name)
{
    DIR* directory;

    // try to open the given directory
    if ((directory = opendir(dirpath)) == NULL)
    {
        // if something goes wrong errno should be changed,
        // so that the perror function is used to see what happaned
        perror("Unable to open the path");
        usage(program_name);
    }
    
    // dirent is a structure containing information 
    // the current object's serial number (inode) and its name
    // p_name is terminated with '\0'
    struct dirent* drnt;

    // stat is a structure containing the given file status
    // e.g. it helps to determine what type of file 
    // are you dealing with
    struct stat statbuf;

    if (be_verbose_flag)
        printf("Scanning directory \"%s\" ...\n", dirpath);

    // change current working directory to the given one
    if (chdir(dirpath) == -1)
    {
        perror("Cannot change current working directory");
        usage(program_name);
    }

    do {
        errno = 0;

        // readdir returns NULL if an error occured or 
        // if there is nothing to read anymore

        // thats why we're using errno to indicate if an error
        // occured

        // try to get another object from the directory
        if ((drnt = readdir(directory)) != NULL)
        {
            // try to get status of the current file
            if (lstat(drnt->d_name, &statbuf) == -1)
            {
                perror("Error during reading a file status");
                usage(program_name);
            }

            scanning_step(drnt->d_name, &statbuf, 0, NULL);
        }
    } while (drnt != NULL);

    // catch the reading error
    if (errno != 0)
    {
        perror("Error during reading the directory");
        usage(program_name);
    }
    
    // try to close the directory
    if (closedir(directory) != 0)
    {
        perror("Unable to close the directory");
        usage(program_name);
    }

    // restore current working directory
    if (chdir(abs_cwd) == -1)
    {
        perror("Cannot change current working directory");
        usage(program_name);
    }
}

// the difference between stat() and lstat() is that
// stat() returns information about the file the link references
// while lstat() returns information about the link

// every directory has hardlink '.' to itself 
// and hardlink '..' to its parent

int main(int argc, char **argv)
{
    char new_arg;
    while ((new_arg = getopt(argc, argv, ":vhrl:")) != -1)
    {
        switch (new_arg)
        {
            case 'v':
                be_verbose_flag = 1;
                break;
            case 'h':
                help_flag = 1;
                break;
            case 'r':
                recursive_flag = 1;
                break;
            case 'l':
                fd_limit = atoi(optarg);
                if (fd_limit == 0)
                {
                    fprintf(stderr, "Option argument for 'l' is incorrect.\n");
                    usage(argv[0]);
                }
                break;
            case ':':
                perror("Missing an argument");
                usage(argv[0]);
                break;
            case '?':
                perror("Option character not defined");
                usage(argv[0]);
                break;
            default:
                break;
        }
    }

    if (help_flag)
    {
        help(argv[0]);
        exit(EXIT_SUCCESS);
    }

    // find max path size
    size_t size;
    long path_max = pathconf(".", _PC_PATH_MAX);

    if (path_max == -1)
        size = 1024;
    else if (path_max > 10240)
        size = 10240;
    else 
        size = path_max; 

    // allocate memory for absolute value of the current working directory
    char *abs_cwd = malloc(size);

    // get current working directory absolute path
    if (getcwd(abs_cwd, size) == NULL)
    {
        perror("Cannot get absolute current working directory");
        usage(argv[0]);
    }

    // if there are no path arguments, scan current working directory
    if (argc - optind == 0)
    {
        // <dot> (".") stands for current working directory
        // directory path is set to current working directory by default
        if (recursive_flag)
        {
            if (nftw(".", scanning_step, fd_limit, FTW_PHYS) == -1)
            {
                perror("Error: nftw() function has failed");
                exit(EXIT_FAILURE);
            }
        }
        else
        {
            scan_dirpath(".", abs_cwd, argv[0]);
        }
    }
    else
    {
        // scan the given directories
        for (int i = optind; i < argc; i++)
        {
            if (recursive_flag)
            {
                if (nftw(argv[i], scanning_step, 20, FTW_PHYS) == -1)
                {
                    perror("Error: nftw() function has failed");
                    exit(EXIT_FAILURE);
                }
            }
            else
            {
                scan_dirpath(argv[i], abs_cwd, argv[0]);
            }
        }
    }

    // print filetypes counts
    printf("Summary: \n");

    for (int i = 0; i < FILETYPES_COUNT; i++)
    {
        printf("  %s: %d\n", filetypes[i], filetypes_counts[i]);
    }

    // free absolute working directory
    free(abs_cwd);

    return EXIT_SUCCESS;
}