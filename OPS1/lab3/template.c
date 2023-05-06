#include <errno.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <memory.h>
#include <signal.h>

#define MAXLINE 4096
#define DEAFULT_THREAD_COUNT 10 
#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))

typedef unsigned int UINT;
typedef struct timespec timespec_t;

void msleep(UINT miliseconds);
void ReadInput(int argc, char **argv, int *threadCount);
void *ThreadFunction(void *rawPtr);

int main(int argc, char **argv)
{
    int threadCount;
    ReadInput(argc, argv, &threadCount);

    // create tids
    pthread_t *tidsArr = (pthread_t*)malloc(sizeof(pthread_t) * threadCount);
    if (tidsArr == NULL)
        ERR("malloc() failed");
        
    // create threads
    for (int i = 0; i < threadCount; i++)
    {
        pthread_create(&tidsArr[i], NULL, ThreadFunction, NULL);
    }

    // wait for each thread 
    for (int i = 0; i < threadCount; i++)
    {
        pthread_join(tidsArr[i], NULL);
    }
    
    // free memory
    free(tidsArr);    

    return EXIT_SUCCESS;
}

void msleep(UINT miliseconds)
{
	time_t sec = (int)(miliseconds / 1000);
	miliseconds = miliseconds - (sec * 1000);
	timespec_t req = { 0 };
	req.tv_sec = sec;
	req.tv_nsec = miliseconds * 1000000L;
	if (nanosleep(&req, &req))
		ERR("nanosleep");
}

void ReadInput(int argc, char **argv, int *threadCount)
{
    *threadCount = DEAFULT_THREAD_COUNT;

    if (argc >= 2)
    {
        int input = atoi(argv[1]);
        if (input > 0)
            *threadCount = input;
    }
}

void *ThreadFunction(void *rawPtr)
{
    printf("*\n");
    return NULL;
}