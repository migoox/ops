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
#define DEAFULT_DURATION 3 
#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))
#define ELAPSED(start, end) ((end).tv_sec - (start).tv_sec) + (((end).tv_nsec - (start).tv_nsec) * 1.0e-9)

typedef unsigned int UINT;
typedef struct timespec timespec_t;

typedef struct PigArg_
{
    int index;
    UINT seed;
    int *pHouse;
    pthread_mutex_t *mtxHouse;
} PigArg;

UINT msleep(UINT miliseconds);
void set_handler(int signal, void (*handler)(int));

void ReadInput(int argc, char **argv, int *threadCount, int *duration);
void *WorkerPig(void *rawPtr);
void WolfSignalHandler(int signal);
void PigsTestament(void *rawPtr);
int Max(int *array, pthread_mutex_t *mutexArr, int size);

int main(int argc, char **argv)
{
    int pigCount;
    int duration;
    ReadInput(argc, argv, &pigCount, &duration);

    // set handler
    set_handler(SIGINT, WolfSignalHandler);

    // prepare signal mask for each thread
    sigset_t mask, oldmask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);

    if (pthread_sigmask(SIG_BLOCK, &mask, &oldmask))
        fprintf(stderr, "pthread_sigmask() failed");

    // generate global seed
    srand(time(NULL));

    // houses array
    int *housesArray = (int*)malloc(sizeof(int) * pigCount);
    if (housesArray == NULL)
        ERR("malloc() failed");

    pthread_mutex_t *mtxHousesArray = 
        (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t) * pigCount);
    if (mtxHousesArray == NULL)
        ERR("malloc() failed");

    // fill houses array and init mutexes
    for (int i = 0; i < pigCount; i++)
    {
        housesArray[i] = rand() % 100 + 1;
        if (pthread_mutex_init(&mtxHousesArray[i], NULL))
            ERR("pthread_mutex_init() failed");
    }

    // create and initialize pigs arguments
    PigArg *pigsArgs = (PigArg*)malloc(sizeof(PigArg) * pigCount);
    if (pigsArgs == NULL)
        ERR("malloc() failed");
        
    for (int i = 0; i < pigCount; i++)
    {
        pigsArgs[i].index = i;
        pigsArgs[i].pHouse = &housesArray[i];
        pigsArgs[i].mtxHouse = &mtxHousesArray[i];
        pigsArgs[i].seed = rand();
    }
    
    // create tids
    pthread_t *tidsArr = (pthread_t*)malloc(sizeof(pthread_t) * pigCount);
    if (tidsArr == NULL)
        ERR("malloc() failed");

    // create threads
    for (int i = 0; i < pigCount; i++)
    {
        if (pthread_create(&tidsArr[i], NULL, WorkerPig, &pigsArgs[i]))
            ERR("pthread_create() failed");
    }

    // restore the old mask
    if (pthread_sigmask(SIG_SETMASK, &oldmask, NULL))
        fprintf(stderr, "pthread_sigmask() failed");

    // main thread loop
    timespec_t start, end;
    if (clock_gettime(CLOCK_REALTIME, &start))
        ERR("clock_gettime() failed");

    while (true)
    {
        int T = rand() % 500 + 101; 
        while ((T = msleep(T)) > 0) {}

        // get power of the blow
        int power = rand() % Max(housesArray, mtxHousesArray, pigCount) + 1;
        // get radius 
        int radius = rand() % pigCount;

        printf("Power: %d\n", power);
        printf("Radius: %d\n", radius);
        for (int i = 0; i < pigCount; i++)
        {
        printf("House[%d] = %d\n", i, housesArray[i]);
        }
        printf("--------------------\n");

        // blow out the houses
        for (int i = pigCount - 1; i >= radius; i--)
        {
            //printf("Wolf: Pig's house %d with %d bricks has been blown out with %d power.\n", i, housesArray[i], power);
            pthread_mutex_lock(&mtxHousesArray[i]);
            if (housesArray[i] != 0 && housesArray[i] < power)
            {
                //printf("Wolf: Pig's house %d has been blown out.\n", i);
                housesArray[i] = 0;
                pthread_cancel(tidsArr[i]);
            }
            pthread_mutex_unlock(&mtxHousesArray[i]);
        }

        if (clock_gettime(CLOCK_REALTIME, &end))
            ERR("clock_gettime() failed");

        // break the loop if time have elapsed
        if (ELAPSED(start, end) >= (double)duration)
            break;
    }

    // cancel all of the pigs
    for (int i = 0; i < pigCount; i++)
    {
        pthread_mutex_lock(&mtxHousesArray[i]);
        if (housesArray[i] != 0)
            pthread_cancel(tidsArr[i]);
        pthread_mutex_unlock(&mtxHousesArray[i]);
    }

    // join each thread 
    for (int i = 0; i < pigCount; i++)
    {
        pthread_join(tidsArr[i], NULL);
    }
    
    // print results of the simulation
    for (int i = 0; i < pigCount; i++)
    {
        printf("House[%d] = %d\n", i, housesArray[i]);
    }
    
    // free memory
    for (int i = 0; i < pigCount; i++)
    {
        if (pthread_mutex_destroy(&mtxHousesArray[i]))
            ERR("pthread_mutex_init() value");
    }

    free(tidsArr);    
    free(pigsArgs);
    free(housesArray);
    free(mtxHousesArray);

    return EXIT_SUCCESS;
}

UINT msleep(UINT miliseconds)
{
	time_t sec = (int)(miliseconds / 1000);
	miliseconds = miliseconds - (sec * 1000);

	timespec_t req = { 0 };
	req.tv_sec = sec;
	req.tv_nsec = miliseconds * 1000000L;
    timespec_t unslept = { 0 };

    int ret = nanosleep(&req, &unslept);

    if (ret != 0)
    {
		return unslept.tv_sec * 1000 + req.tv_nsec / 1000000L;
    }

    return 0;
}

void ReadInput(int argc, char **argv, int *threadCount, int *duration)
{
    *threadCount = DEAFULT_THREAD_COUNT;
    if (argc >= 2)
    {
        int input = atoi(argv[1]);
        if (input > 0)
            *threadCount = input;
    }

    *duration = DEAFULT_DURATION;
    if (argc >= 3)
    {
        int input = atoi(argv[2]);
        if (input > 0)
            *duration = input;
    }
}

void *WorkerPig(void *rawPtr)
{
    PigArg *args = (PigArg*)rawPtr;

    pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);
    pthread_cleanup_push(PigsTestament, args);
    while (1)
    {
        int T = rand_r(&args->seed) % 40 + 11; 
        msleep(T);

        // re-enforce the house
        pthread_mutex_lock(args->mtxHouse);
        if (*args->pHouse != 0)
            *args->pHouse += 1;
        pthread_mutex_unlock(args->mtxHouse);
    }
    pthread_cleanup_pop(0);
    return NULL;
}

void WolfSignalHandler(int signal)
{
    if (signal == SIGINT)
    {
        printf("Wolf: SIGINT received, hihi\n");
    }
}

void PigsTestament(void *rawPtr)
{
    kill(getpid(), SIGINT);
}

int Max(int *array, pthread_mutex_t *mutexArr, int size)
{
    if (size == 0)
        return -1;

    pthread_mutex_lock(&mutexArr[0]);
    int maxVal = array[0];
    pthread_mutex_unlock(&mutexArr[0]);

    for (int i = 1; i < size; i++)
    {
        pthread_mutex_lock(&mutexArr[i]);
        if (maxVal < array[i])
        {
            maxVal = array[i];
        }
        pthread_mutex_unlock(&mutexArr[i]);
    }
    return maxVal;
}

void set_handler(int signal, void (*handler)(int))
{
    struct sigaction sa;

    // initalizing all of the members with zero
    memset(&sa, 0, sizeof(struct sigaction));

    // setting handler 
    sa.sa_handler = handler;

    if (sigaction(signal, &sa, NULL) == -1)
    {
        ERR("sigaction() failed");
    }
}
