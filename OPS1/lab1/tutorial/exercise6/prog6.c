#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// from unistd.h
//   int opterr -> if it's non zero getopt prints error
//   int optopt -> uknown option character stored

// usage function, called when an error ocurred
void usage(const char* pname)
{
    fprintf(stderr, "USAGE:%s ([-t x] -n Name) ... \n", pname);
    exit(EXIT_FAILURE);
}

int main(int argc, char** argv)
{
    char new_arg;
    int count = 1;

    // if we set opterr to 0, getopt won't return '?' sign when
    // an error occures

    // there is also long option begining with "--" (getopt_long)
    // but it's GNU standard, not POSIX

    // getopt returns -1 when no more option arguments are available
    // <colon>, ":" indicates that option has argument(s)
    // double <colon>, "::" indicates that option has optional argument(s)
    while ((new_arg = getopt(argc, argv, "n:t:")) != -1)
    {
        if (new_arg == 't')
        {   
            // optarg is a static buffer containing option argument(s)
            count = atoi(optarg);
            continue;
        }
        else if (new_arg == 'n')
        {
            for (int i = 0; i < count; i++)
            {
                printf("Hello %s\n", optarg);
            }
            continue;
        }

        // incorrect option or option's argument list (?)
        usage(argv[0]);
    }

    // optind is set by getopt to the index of the next element of the
    // argv to be processed. If this index is not equal the size of 
    // argv, it means that something wrong happend in the loop
    if (argc > optind)
        usage(argv[0]);

    return EXIT_SUCCESS;
}