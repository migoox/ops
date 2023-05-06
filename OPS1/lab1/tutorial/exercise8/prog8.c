#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// maximum length of the user's input
#define MAX_LENGTH 20

int main(int argc, char **argv)
{
	char name[MAX_LENGTH + 2]; 
    int count = 1;

    char* envtimes = getenv("TIMES");
    if (envtimes != NULL)
    {
        count = atoi(envtimes);
    }

	while (fgets(name, MAX_LENGTH + 2, stdin) != NULL)
    {
        // \n sign is not reauired since it is stored in name 
        // (if line has length <= MAX_LENGTH)
        for (int i = 0; i < count; i++)
        {
		    printf("Hello %s", name);
        }

        // we dont want to count '\n'
        printf("String size: %lu\n", strlen(name) - 1);
    }

    // RESULT wont be visible from the different shell
	if (putenv("RESULT=Done") != 0) 
    {
		fprintf(stderr, "putenv failed");
		return EXIT_FAILURE;
    }

    // if system func is called the child shell is created
    
	return EXIT_SUCCESS;
}