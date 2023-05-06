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

typedef struct Counter_
{
    int value;
    pthread_mutex_t mxValue;
    int threadCheckedCount;
    pthread_mutex_t mxThreadCheckedCount;
} Counter;

typedef struct ThreadArgs_
{
    // seed, uniqe for each thread
    UINT seed;

    // reference to the counter
    Counter *pCounter;

    // end of the program
    bool *pEnd;
} ThreadArgs;

void msleep(UINT miliseconds);
void ReadInput(int argc, char **argv, int *threadCount);
void *ThreadFunction(void *rawPtr);
void *SignalHandlerThreadFunction(void *rawPtr);

int main(int argc, char **argv)
{
    int threadCount;
    ReadInput(argc, argv, &threadCount);
    
    // ignore C-c in whole process
    sigset_t sigset;
    sigemptyset(&sigset);
    sigaddset(&sigset, SIGINT);
    sigprocmask(SIG_BLOCK, &sigset, NULL);

    // initialize counter 
    Counter counter; 
    counter.threadCheckedCount = 0;
    counter.value = 0;
    if (pthread_mutex_init(&counter.mxThreadCheckedCount, NULL))
        ERR("pthread_mutex_init() failed");
    if (pthread_mutex_init(&counter.mxValue, NULL))
        ERR("pthread_mutex_init() failed");

    // set global seed
    srand(time(NULL));

    // signal handler thread
    pthread_t signalHandlerThreadTID;
    bool end = false;
    if (pthread_create(&signalHandlerThreadTID, NULL, SignalHandlerThreadFunction, &end))
        ERR("pthread_create() failed");

    // create tids array
    pthread_t *tidsArr;
    tidsArr = (pthread_t*)malloc(sizeof(pthread_t) * threadCount);
    if (tidsArr == NULL)
        ERR("malloc() failed");

    // create and initialize thread args
    ThreadArgs *threadArgsArr;
    threadArgsArr = (ThreadArgs*)malloc(sizeof(ThreadArgs) * threadCount);
    if (threadArgsArr == NULL)
        ERR("malloc() failed");

    for (int i = 0; i < threadCount; i++)
    {
        threadArgsArr[i].seed = rand();
        threadArgsArr[i].pCounter = &counter;
        threadArgsArr[i].pEnd = &end;
    }

    // create threads
    for (int i = 0; i < threadCount; i++)
    {
        if (pthread_create(&tidsArr[i], NULL, ThreadFunction, &threadArgsArr[i]))
            ERR("pthread_create() failed");
    }

    // main loop of the program
    while (end == false)
    {
        msleep(100);
        if ((int)(counter.threadCheckedCount / threadCount) > counter.value)
        {
            pthread_mutex_lock(&counter.mxValue);
            counter.value++;
            pthread_mutex_unlock(&counter.mxValue);
        }
    }

    // joining each thread 
    for (int i = 0; i < threadCount; i++)
    {
        pthread_join(tidsArr[i], NULL);
    }
    
    // joining signal handler thread
    pthread_join(signalHandlerThreadTID, NULL);

    // free memory
    pthread_mutex_destroy(&counter.mxThreadCheckedCount);
    pthread_mutex_destroy(&counter.mxValue);
    free(tidsArr);    
    free(threadArgsArr);    

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
    ThreadArgs* args = (ThreadArgs*) rawPtr;
    int M = rand_r(&args->seed) % 99 + 2;

    int prevCounterVal = -1;
    int counterVal = 0;
    while (*args->pEnd == false)
    {
        pthread_mutex_lock(&args->pCounter->mxValue);
        counterVal = args->pCounter->value;
        pthread_mutex_unlock(&args->pCounter->mxValue); 
        if (prevCounterVal != counterVal)
        {
            if (counterVal % M == 0)
                printf("TID: %ld --> L=%d is divisible by M=%d\n", 
                    pthread_self(), counterVal, M);

            pthread_mutex_lock(&args->pCounter->mxThreadCheckedCount);
            args->pCounter->threadCheckedCount++;
            pthread_mutex_unlock(&args->pCounter->mxThreadCheckedCount);   

            prevCounterVal = counterVal;
        }     
    }

    return NULL;
}

void *SignalHandlerThreadFunction(void *rawPtr)
{
    bool *end = (bool*)rawPtr;

    sigset_t sigset;
    sigemptyset(&sigset);
    sigaddset(&sigset, SIGINT);

    int sig;
    // wait for SIGINT
    int ret = sigwait(&sigset, &sig);
    if (ret == 0) 
    {
        if (sig == SIGINT)
        {
            printf("\nReceived signal C-c\n"); 
            *end = true;
        }
    } 
    else 
    {
        printf("sigwait failed with error %d\n", ret);
    }
    return NULL;
}