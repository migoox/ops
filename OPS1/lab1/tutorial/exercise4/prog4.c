#include <stdio.h>
#include <stdlib.h>

#define MAX_LENGTH 20

// to run this program with arguments use one of the following methods

// [Method 1]
// $ ./prog4 arg1 arg2 arg3 ...

// [Method 2]
// $ xargs ./prog4
// $ arg1
// $ arg2
// $ ... 
// $ C^D

// [Method 3]
// $ xargs -a file_containing_args.txt ./prog4
// 'a' option stands for reading from file (--arg-file=file) 

// If you wish "blank fields" like spaces or next-lines to be ommited
// you can use -0 option - xargs will seek for '\0' end-of-string
// [Method 4]
// $ xargs -a file_containing_args.txt -0 ./prog4

// [Method 5] (does same thing that Method 3)
// $ cat file_containing_args.txt | xargs ./prog4

// [Method 6] (safer than Method 4 since we can't 
// be sure that "\0" is used as we would expect)
// $ cat file_containing_args.txt | tr "\n" "\0" | xargs -0 ./prog4

int main(int argc, char** argv)
{
    printf("Program name: %s\n", argv[0]);
    printf("Given arguments: \n");
    for (int i = 1; i < argc; i++)
    {
        printf("%s\n", argv[i]);      
    }

    return EXIT_SUCCESS;
}