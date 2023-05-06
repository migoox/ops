#include <pthread.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAXLINE 4096
// deafult values
#define DEFAULT_N 1000
#define DEFAULT_K 10

// bins 
#define BIN_COUNT 11

// macro used in throwBall function, returns random double in [0, 1]
#define NEXT_DOUBLE(seedptr) ((double)rand_r(seedptr) / (double)RAND_MAX)
#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))

typedef unsigned int UINT;

// structure used to parse arguments to a thread function 
typedef struct argsThrower {
	pthread_t tid;
	UINT seed;
	int *pBallsThrown;
	int *pBallsWaiting;
	int *bins;
	pthread_mutex_t *mxBins;
	pthread_mutex_t *pmxBallsThrown;
	pthread_mutex_t *pmxBallsWaiting;
} argsThrower_t;

// reads arguments from the console, uses deafult values if nothing has been specified
void ReadArguments(int argc, char **argv, int *ballsCount, int *throwersCount);

// creates threads with arguments specified by argsArray 
// each thread uses thread attribute - PTHREAD_CREATE_DETACHED
//
// A value of PTHREAD_CREATE_DETACHED shall cause all threads 
// created with attr to be in the detached state, whereas using a value of 
// PTHREAD_CREATE_JOINABLE shall cause all threads created with attr to 
// be in the joinable state. The default value of the detachstate attribute 
// shall be PTHREAD_CREATE_JOINABLE.
// When a thread is marked as detached, 
// it will automatically release all of its resources (such as memory) when it terminates
void MakeThrowers(argsThrower_t *argsArray, int throwersCount);

// Thread function responsible for throwing balls
void *Thrower(void *args);

// Function which returns result of the throw
int ThrowBall(UINT *seedptr);

int main(int argc, char **argv)
{
    // reading input values
	int ballsCount, throwersCount;
	ReadArguments(argc, argv, &ballsCount, &throwersCount);

    // initialize values shared between processes
	int ballsThrown = 0;
	int ballsWaiting = ballsCount;
    // initialiing mutexes with no error checking
	pthread_mutex_t mxBallsThrown = PTHREAD_MUTEX_INITIALIZER;
	pthread_mutex_t mxBallsWaiting = PTHREAD_MUTEX_INITIALIZER;

	int bins[BIN_COUNT];
	pthread_mutex_t mxBins[BIN_COUNT];
	for (int i = 0; i < BIN_COUNT; i++) {
        // setting all bins to be empty
		bins[i] = 0;

        // initialiing mutexes with error checking (equivalent to the above one)
		if (pthread_mutex_init(&mxBins[i], NULL))
			ERR("Couldn't initialize mutex!");
	}

    // allocating memory for arguments for each thread
	argsThrower_t *args = (argsThrower_t *)malloc(sizeof(argsThrower_t) * throwersCount);
	if (args == NULL)
		ERR("Malloc error for throwers arguments!");
    
    // setting up a seed
	srand(time(NULL));

    // initializng thread arguments, by giving the references to 
    // the shared values
	for (int i = 0; i < throwersCount; i++) {
		args[i].seed = (UINT)rand();
		args[i].pBallsThrown = &ballsThrown;
		args[i].pBallsWaiting = &ballsWaiting;
		args[i].bins = bins;
		args[i].pmxBallsThrown = &mxBallsThrown;
		args[i].pmxBallsWaiting = &mxBallsWaiting;
		args[i].mxBins = mxBins;
	}

    // pthread_create with PTHREAD_CREATE_DETACHED attribute
	MakeThrowers(args, throwersCount);

    // waiting in the main thread for all of the balls being thrown
    // by the other threads
    // this test is required, since there is no joining
    int bt = 0;
	while (bt < ballsCount) 
    {
		sleep(1);
		pthread_mutex_lock(&mxBallsThrown);
		bt = ballsThrown;
		pthread_mutex_unlock(&mxBallsThrown);
	}

    // counting real balls count and getting mean value
	int realBallsCount = 0;
	double meanValue = 0.0;
	for (int i = 0; i < BIN_COUNT; i++) {
		realBallsCount += bins[i];
		meanValue += bins[i] * i;
	}
	meanValue = meanValue / realBallsCount;

    // printing results
	printf("Bins count:\n");
	for (int i = 0; i < BIN_COUNT; i++)
		printf("%d\t", bins[i]);
    printf("\nReal balls count: %d", ballsCount);
	printf("\nTotal balls count: %d\nMean value: %f\n", realBallsCount, meanValue);

	// for (int i = 0; i < BIN_COUNT; i++) pthread_mutex_destroy(&mxBins[i]);
	// free(args);
	// The resources used by detached threads cannod be freed as we are not sure
	// if they are running yet.
	exit(EXIT_SUCCESS);
}

void ReadArguments(int argc, char **argv, int *ballsCount, int *throwersCount)
{
	*ballsCount = DEFAULT_N;
	*throwersCount = DEFAULT_K;
	if (argc >= 2) {
		*ballsCount = atoi(argv[1]);
		if (*ballsCount <= 0) {
			printf("Invalid value for 'balls count'");
			exit(EXIT_FAILURE);
		}
	}
	if (argc >= 3) {
		*throwersCount = atoi(argv[2]);
		if (*throwersCount <= 0) {
			printf("Invalid value for 'throwers count'");
			exit(EXIT_FAILURE);
		}
	}
}

void MakeThrowers(argsThrower_t *argsArray, int throwersCount)
{
    // creating and initializing thread attributes
	pthread_attr_t threadAttr;
	if (pthread_attr_init(&threadAttr))
		ERR("Couldn't create pthread_attr_t");

    // setting detach state (PTHREAD_CREATE_JOINABLE by deafult)
	if (pthread_attr_setdetachstate(&threadAttr, PTHREAD_CREATE_DETACHED))
		ERR("Couldn't setdetachsatate on pthread_attr_t");

    // creating ne threads
	for (int i = 0; i < throwersCount; i++) 
    {
		if (pthread_create(&argsArray[i].tid, &threadAttr, Thrower, &argsArray[i]))
			ERR("Couldn't create thread");
	}

    // deallocating thread attribute container
	pthread_attr_destroy(&threadAttr);
}

void *Thrower(void *voidArgs)
{
    // casting a raw pointer 
	argsThrower_t *args = voidArgs;
    uint count = 0;
	while (1) 
    {
        // checking if all balls has been thrown, if so break the loop
		pthread_mutex_lock(args->pmxBallsWaiting);
		if (*args->pBallsWaiting > 0) 
        {
			(*args->pBallsWaiting) -= 1;
			pthread_mutex_unlock(args->pmxBallsWaiting);
		} 
        else 
        {
			pthread_mutex_unlock(args->pmxBallsWaiting);
			break;
		}

        // getting the result of throwing the ball
		int binno = ThrowBall(&args->seed);

        // updating bin 
		pthread_mutex_lock(&args->mxBins[binno]);
		args->bins[binno] += 1;
		pthread_mutex_unlock(&args->mxBins[binno]);

        // updating balls thrown 
		pthread_mutex_lock(args->pmxBallsThrown);
		(*args->pBallsThrown) += 1;
		pthread_mutex_unlock(args->pmxBallsThrown);
        count++;
	}

    printf("I am thread with TID: %ld, I've thrown %d balls.\n",args->tid, count);
	return NULL;
}

/* returns # of bin where ball has landed */
int ThrowBall(UINT *seedptr)
{
	int result = 0;
	for (int i = 0; i < BIN_COUNT - 1; i++)
		if (NEXT_DOUBLE(seedptr) > 0.5)
			result++;
	return result;
}
