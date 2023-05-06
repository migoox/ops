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

#define MAXLINE 4096
#define YEARS 4
#define DEFAULT_STUDENT_COUNT 100
#define ELAPSED(start, end) ((end).tv_sec - (start).tv_sec) + (((end).tv_nsec - (start).tv_nsec) * 1.0e-9)
#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))

typedef unsigned int UINT;

typedef struct timespec timespec_t;

typedef struct StudentsList_ {
	bool *presenceArr;
	pthread_t *tidsArr;
	int count;
	int presentCount;
} StudentsList;

typedef struct YearCounters_ {
	int values[YEARS];
	pthread_mutex_t mxCounters[YEARS];
} YearCounters;

typedef struct StudentArgs_ {
	YearCounters *pYearCounters;
	int year;
} StudentArgs;

void ReadArguments(int argc, char **argv, int *studentsCount);
void *student_life(void *rawPtr);
void increment_counter(StudentArgs *args);
void decrement_counter(void *_args);
void msleep(UINT milisec);
void kick_student(StudentsList *studentsList);

int main(int argc, char **argv)
{
	int studentsCount;
	ReadArguments(argc, argv, &studentsCount);

	// initialize shared between threads year counters
	YearCounters yearCounters;
	for (int i = 0; i < YEARS; i++)
	{
		yearCounters.values[i] = 0;
		if (pthread_mutex_init(&yearCounters.mxCounters[i], NULL))
			ERR("pthread_mutex_init() failed");
	}

	// create and initialize student list structure
	StudentsList studentsList;
	studentsList.count = studentsCount;
	studentsList.presentCount = studentsCount;
	studentsList.tidsArr = (pthread_t *)malloc(sizeof(pthread_t) * studentsCount);
	studentsList.presenceArr = (bool *)malloc(sizeof(bool) * studentsCount);

	if (studentsList.tidsArr == NULL || studentsList.presenceArr == NULL)
	{
		ERR("malloc() failed");
	}

	for (int i = 0; i < studentsCount; i++)
	{
		studentsList.presenceArr[i] = false;
	}
	
	for (int i = 0; i < studentsCount; i++)
	{
		if (pthread_create(&studentsList.tidsArr[i], NULL, student_life, &yearCounters))
			ERR("Failed to create student thread!");
	}

	// set global seed
	srand(time(NULL));

	// main loop
	timespec_t start, end;
	if (clock_gettime(CLOCK_REALTIME, &start))
		ERR("clock_gettime() failed");

	while (1)
	{
		msleep(rand() % 201 + 100);

		if (clock_gettime(CLOCK_REALTIME, &end))
			ERR("clock_gettime() failed");

		// generate random student id and kick (cancel the thread)
		kick_student(&studentsList);

		// break the loop if time have elapsed
		if (ELAPSED(start, end) >= 4.0)
			break;
	}

	// join all of the threads
	for (int i = 0; i < studentsCount; i++)
	{
		if (pthread_join(studentsList.tidsArr[i], NULL))
			ERR("Failed to join with a student thread!");
	}


	printf(" First year: %d\n", yearCounters.values[0]);
	printf("Second year: %d\n", yearCounters.values[1]);
	printf(" Third year: %d\n", yearCounters.values[2]);
	printf("  Engineers: %d\n", yearCounters.values[3]);
	free(studentsList.presenceArr);
	free(studentsList.tidsArr);
	exit(EXIT_SUCCESS);
}

void ReadArguments(int argc, char **argv, int *studentsCount)
{
	*studentsCount = DEFAULT_STUDENT_COUNT;
	if (argc >= 2) {
		*studentsCount = atoi(argv[1]);
		if (*studentsCount <= 0) {
			printf("Invalid value for 'studentsCount'");
			exit(EXIT_FAILURE);
		}
	}
}

void *student_life(void *voidArgs)
{
	// PTHREAD_CANCEL_ASYNCHRONOUS would require to use async-safe functions only
	// here cancellation requests are held pending until a cancellation point is reached 
	pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);

	StudentArgs args;
	args.pYearCounters = (YearCounters*)voidArgs;

	for (args.year = 0; args.year < YEARS - 1; args.year++) 
	{
		// increment the current year state
		increment_counter(&args);

		// push cleanup routine to the stack of cleanup routines
		// if this thread will be canceled, the year counter will be updated
		// it allows to remove student safely
		// push and pop should be paired in the same scope
		pthread_cleanup_push(decrement_counter, &args);

		msleep(1000);

		// if cancelation didn't happen pop the routine and activate it here
		pthread_cleanup_pop(1);
	}

	increment_counter(&args);
	
	return NULL;
}

void increment_counter(StudentArgs *args)
{
	pthread_mutex_lock(&(args->pYearCounters->mxCounters[args->year]));
	args->pYearCounters->values[args->year] += 1;
	pthread_mutex_unlock(&(args->pYearCounters->mxCounters[args->year]));
}

void decrement_counter(void *rawArgs)
{
	StudentArgs *args = rawArgs;
	pthread_mutex_lock(&(args->pYearCounters->mxCounters[args->year]));
	args->pYearCounters->values[args->year] -= 1;
	pthread_mutex_unlock(&(args->pYearCounters->mxCounters[args->year]));
}

void msleep(UINT milisec)
{
	time_t sec = (int)(milisec / 1000);
	milisec = milisec - (sec * 1000);
	timespec_t req = { 0 };
	req.tv_sec = sec;
	req.tv_nsec = milisec * 1000000L;
	if (nanosleep(&req, &req))
		ERR("nanosleep");
}

void kick_student(StudentsList *studentsList)
{
	if (studentsList->presentCount == 0)
		return;
	
	int cInd = rand() % studentsList->count;
	while (studentsList->presenceArr[cInd] == true)
	{
		// round robin
		cInd = (cInd + 1) % (studentsList->count);
	}

	// cancel the thread
	pthread_cancel(studentsList->tidsArr[cInd]);

	// update presence array
	studentsList->presenceArr[cInd] = true;

	// update present students count
	studentsList->presentCount--;
}