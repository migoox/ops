#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAXLINE 4096
// deafult value
#define DEFAULT_K 10
#define DELETED 0

// macro used in throwBall function, returns random double in [0, 1]
#define NEXT_DOUBLE(seedptr) ((double)rand_r(seedptr) / (double)RAND_MAX)
#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))
#define SAFE(source) if (source) { ERR(#source); }

typedef struct SignalHandlerThreadArgs_
{
	pthread_t tid;
	uint seed;
	int* array;
	int* size;
	int* quitFlag;
	sigset_t *pMask;
	pthread_mutex_t* arrayMutexPtr;
	pthread_mutex_t* quitFlagMutexPtr;
} SignalHandlerThreadArgs;

// reads arguments from the console, uses deafult values if nothing has been specified
void ReadArguments(int argc, char **argv, int *size);
void* SignalHandlerThreadFunction(void* rawPtr);

int main(int argc, char **argv)
{
	// setting global seed value
	srand(time(NULL));

    // reading input values
	int size;
	ReadArguments(argc, argv, &size);

	// init list of numbers
	int *array = (int*)malloc(sizeof(int) * size);
	for (uint i = 0; i < size; i++)
	{
		array[i] = i + 1;
	}
	int quitFlag = 0;

	// init mutexes
	pthread_mutex_t arrayMutex = PTHREAD_MUTEX_INITIALIZER;
	pthread_mutex_t quitFlagMutex = PTHREAD_MUTEX_INITIALIZER;

	// create mask for the process
	sigset_t oldMask, newMask;
	sigemptyset(&newMask);
	sigaddset(&newMask, SIGINT);
	sigaddset(&newMask, SIGQUIT);
	// sigprocmask can be used as well
	if (pthread_sigmask(SIG_BLOCK, &newMask, &oldMask))
		ERR("SIG_BLOCK error");

	// create thread arguments container
	SignalHandlerThreadArgs args;
	args.seed = rand();
	args.array = array;
	args.size = &size;
	args.quitFlag = &quitFlag;
	args.arrayMutexPtr = &arrayMutex;
	args.quitFlagMutexPtr = &quitFlagMutex;
	args.pMask = &newMask;

	// create a thread
    if (pthread_create(&args.tid, NULL, SignalHandlerThreadFunction, &args))
		ERR("pthread_create() failed");
	
	// printing
	int index = 0;
	while (1)
	{
		sleep(1);
		pthread_mutex_lock(&quitFlagMutex);
		if (quitFlag != 0)
		{
			pthread_mutex_unlock(&quitFlagMutex);
			break;
		}
		else
		{
			pthread_mutex_unlock(&quitFlagMutex);
		}
		if (index < size)
		{
			pthread_mutex_lock(&arrayMutex);
			
			while (array[index] == DELETED)
				index++;
			
			printf("Number: %d\n", array[index]);
			pthread_mutex_unlock(&arrayMutex);
			index++;
		}
	}

	pthread_join(args.tid, NULL);

	pthread_mutex_destroy(&arrayMutex);
	pthread_mutex_destroy(&quitFlagMutex);
	free(array);

	if (pthread_sigmask(SIG_UNBLOCK, &newMask, &oldMask))
		ERR("SIG_BLOCK error");

	return EXIT_SUCCESS;
}

void ReadArguments(int argc, char **argv, int *size)
{
	*size = DEFAULT_K;
	if (argc >= 2) {
		*size = atoi(argv[1]);
		if (*size <= 0) {
			printf("Invalid value for 'size'");
			exit(EXIT_FAILURE);
		}
	}
}

void* SignalHandlerThreadFunction(void* rawPtr)
{
	SignalHandlerThreadArgs* args = (SignalHandlerThreadArgs*)rawPtr;

	while(1)
	{
		int sig;
		if (sigwait(args->pMask, &sig))
			ERR("sigwait failed.");

		// signal has been received
		if (sig == SIGINT)
		{
			if ((*args->size) > 0)
			{
				int delIndex = rand_r(&args->seed) % (*args->size);

				pthread_mutex_lock(args->arrayMutexPtr);
				while (delIndex < (*args->size) && args->array[delIndex] == DELETED)
				{
					delIndex++;
				}
				args->array[delIndex] = DELETED;
				pthread_mutex_unlock(args->arrayMutexPtr);
			}
		}
		else // sig == SIGQUIT
		{
			pthread_mutex_lock(args->quitFlagMutexPtr);
			(*args->quitFlag) = 1;
			pthread_mutex_unlock(args->quitFlagMutexPtr);
			break;
		}
	}

    return NULL;
}
