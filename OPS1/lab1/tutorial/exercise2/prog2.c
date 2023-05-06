#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// perror() function produces a message on stderr describing the last 
// error encountered during a call to a system or library function.
// The message produced contains given const char*, colon and the error created
// by a system/library function called. In this program, we are not using
// any system function call so even if the ERR is called the output after
// colon will be "Success". 

// fprintf() function allows changing a stream. We want to print additional
// info about the error so we are using stderr stream. 

// Constants __FILE__ and __LINE__ store respectively the file and the line in which 
// this macro was called 
#define ERR(source) (perror(source), \
fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), \
exit(EXIT_FAILURE))

// maximum length of the user's input
#define MAX_LENGTH 20

int main(int argc, char **argv)
{
    // We increase capacity of the storage by 2. One char is needed for '\0' and 
    // the another one for determining if the user has given more than 
    // expected characters count
    char name[MAX_LENGTH + 2];

    // create format for MAX_LENGTH
    char format[10];
    sprintf(format, "%%%ds", MAX_LENGTH + 1);

    // use format and get name"%%%ds", MAX_LENGTH + 1);

    // use format an from the user
    scanf(format, name);

    // strlen() function requires string.h
    if (strlen(name) > MAX_LENGTH) ERR("Name is too long");

    printf("Hello %s\n", name);

    return EXIT_SUCCESS; 
}