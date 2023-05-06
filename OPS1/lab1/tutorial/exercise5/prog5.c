#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define MAX_LENGTH 20

#define ERROR(source) ( \
fprintf(stderr, "[ERROR]"), \
fprintf(stderr, "\tMessage: %s\n\t", source), \
perror("Value of errno"), \
fprintf(stderr, "\tLocation: %s:%d\n", __FILE__, __LINE__), \
exit(EXIT_FAILURE))

void usage(char *pname)
{
	fprintf(stderr, "USAGE:%s name times>0\n", pname);
	exit(EXIT_FAILURE);
}

int main(int argc, char** argv)
{
    // too many or not enough input arguments
    if (argc != 3)
    {
        usage(argv[0]);
    }

    // the first argument should be a string containing the name
    // if it contain number error will be returned
    char* ptr1 = strpbrk(argv[1], "0123456789");
    if (ptr1 != NULL)
    {
        ERROR("Arg1 shouldn't contain any numbers"); 
    }

    // the second argument should be an long
    // strtol has two error codes: EINVAL and ERANGE
    // errno == EINVAL the given base contains unsupported value
    // errno == ERANGE the resulting number is out of range

    // setting errno to 0 to distinguish success/failure after call
    errno = 0;

    // if the second argument of strtol() is not NULL,
    // the function will store the address of the first
    // invalid character in the given pointer (endptr)
    char* endptr = NULL;

    long count = strtol(argv[2], &endptr, 10);

    if (errno != 0 || endptr == argv[2] || *endptr != '\0' || count < 0)
    {
        // Warning: seqence of conditions matters.

        // errno != 0           --> EINVAL and ERANGE errors
        // endptr == argv[2]    --> string is blank
        // *endptr != '\0'      --> invalid character found
        // count < 0            --> the number is not positive

        ERROR("Arg2 should be a positive number from a long range"); 
    }

    for (long i = 0; i < count; i++)
    {
        printf("%s\n", argv[1]);
    }
    
    // an alternative would be using atoi, which returns 0
    // every time it cannot convert an input 
    
    return EXIT_SUCCESS;
}