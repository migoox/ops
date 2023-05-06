#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// maximum length of the user's input
#define MAX_LENGTH 20

// program can be used with file input,
// in order to do that, use one of the following methods
// $ ./prog3 < data.txt
// $ cat data.txt | ./prog3

int main(int argc, char **argv)
{
    // note that we adding 2 to max length
    // first sign is needed for '\n' and the second one for '\0'
	char name[MAX_LENGTH + 2];

    // fgets() function reads bytes from stream into the given array
    // until it sees '\n' or n - 1 bytes are read or end-of-file condition
    // is encountered. It reads null byte '\0'. 

    // fgets() returns NULL if it sees the end-of-file indicator (for
    // stdin it can be sent using Ctrl + D).
    // If an error occurs, the errno value is set and NULL is returned.

	while (fgets(name, MAX_LENGTH + 2, stdin) != NULL)
    {
        // \n sign is not reauired since it is stored in name 
		printf("Hello %s", name);

        // we dont want to count '\n'
        printf("String size: %lu\n", strlen(name) - 1);
    }

    // Warning: if line input chars count exceeds MAX_LENGTH, it will be
    // separated into more inputs with only one having '\n' sign 
	return EXIT_SUCCESS;
}