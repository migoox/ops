#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

#define FILETYPES_COUNT 8

// the difference between prog9.c and prog10.c is that
// prog9.c doesn't change current working directory, and allocates
// memory for strings to new paths instead
// also prog10.c allows recursive scan

// option flags
static int be_verbose_flag = 0;
static int help_flag = 0;

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
}

static void scan_dirpath(const char* dirpath, const char* program_name)
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

    int index = 1;
    do {
        errno = 0;

        // readdir returns NULL if an error occured or 
        // if there is nothing to read anymore

        // thats why we're using errno to indicate if an error
        // occured

        // try to get another object from the directory
        if ((drnt = readdir(directory)) != NULL)
        {
            if (be_verbose_flag)
                printf("[%d]: %s", index, drnt->d_name);

            // create container for the path to current element
            // one additinal memory cell for '\0'
            // and one for '/'
            char* elementpath = 
                (char*)malloc(strlen(dirpath) + strlen(drnt->d_name) + 2);
            
            // copy dirpath to the container
            strcpy(elementpath, dirpath);

            // move terminating sign by one memory cell
            elementpath[strlen(elementpath) + 1] = '\0'; 
            
            // append '/' sign
            elementpath[strlen(elementpath)] = '/'; 
            
            // append name of the file
            strcat(elementpath, drnt->d_name);

            // find the status
            int lstat_result = lstat(elementpath, &statbuf);

            // free the memory
            free(elementpath);

            // try to get status of the current file
            if (lstat_result == -1)
            {
                perror("Error during reading a file status");
                usage(program_name);
            }

            // this is temp value which helps to determine
            // either the file type exists 
            int uknown = 1;
            
            // loop through the file types
            for (int i = 0; i < FILETYPES_COUNT - 1; i++)
            {
                if ((statbuf.st_mode & S_IFMT) == filetypes_macros[i])
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
        }
        index++;
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
}

// the difference between stat() and lstat() is that
// stat() returns information about the file the link references
// while lstat() returns information about the link

// every directory has hardlink '.' to itself 
// and hardlink '..' to its parent

int main(int argc, char **argv)
{
    char new_arg;
    while ((new_arg = getopt(argc, argv, "vh")) != -1)
    {
        switch (new_arg)
        {
            case 'v':
                be_verbose_flag = 1;
                break;
            case 'h':
                help_flag = 1;
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

    // if there are no path arguments, scan current working directory
    if (argc - optind == 0)
    {
        // <dot> (".") stands for current working directory
        // directory path is set to current working directory by default
        scan_dirpath(".", argv[0]);
    }
    else
    {
        // scan the given directories
        for (int i = optind; i < argc; i++)
        {
            scan_dirpath(argv[i], argv[0]);
        }
    }

    // print filetypes counts
    printf("Summary: \n");

    for (int i = 0; i < FILETYPES_COUNT; i++)
    {
        printf("  %s: %d\n", filetypes[i], filetypes_counts[i]);
    }

    return EXIT_SUCCESS;
}