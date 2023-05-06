#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <memory.h>
#include <string.h>
#include <math.h>
#include <errno.h>

#define ERR(source)                                                                                                    \
	(fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), \
    perror(source), exit(EXIT_FAILURE))

void usage(char *name)
{
	fprintf(stderr, "USAGE: %s n k\n", name);
    fprintf(stderr, "k - threads count");
    fprintf(stderr, "n - samples per thread count");
	exit(EXIT_FAILURE);
}

typedef struct threadInfo
{
    pthread_t tid;
    uint seed;
    int samplesCount;
} threadInfo_t;

void* worker(void* args);

int main(int argc, char **argv)
{
    srand(time(NULL));

    if (argc != 3)
        usage(argv[0]);

    int k = atoi(argv[1]);
    if (k <= 0) usage(argv[0]);

    int n = atoi(argv[2]);
    if (n <= 0) usage(argv[0]);

    threadInfo_t* info = (threadInfo_t*) malloc(sizeof(threadInfo_t) * n);
    if (info == NULL) ERR("malloc() failed");

    for (int i = 0; i < k; i++)
    {
        info[i].seed = rand();
        info[i].samplesCount = n;
        pthread_create(&(info[i].tid), NULL, worker, &(info[i]));
    }

    double estimated = 0.0;
    double *val;
    for (int i = 0; i < k; i++)
    {

        int err = pthread_join(info[i].tid, (void**) &val);
        if (err != 0) ERR("pthread_join failed");
        if (NULL != val)
        {
            estimated += *val;
            free(val);
        }
    }
    estimated = estimated / (double)k;
    printf("PI ~= %f\n", estimated);

    free(info);

    return EXIT_SUCCESS;
}

void* worker(void* args)
{
    threadInfo_t* info = (threadInfo_t*)args;
    
    double* result = (double*)malloc(sizeof(double));
    if (result == NULL) ERR("malloc() failed");   

    int insideTheCircleCount = 0;
    for (int i = 0; i < info->samplesCount; i++)
    {
        // generating random normalized coords
        double xCoord = (double)(rand_r(&info->seed)) / (double)RAND_MAX;
        double yCoord = (double)(rand_r(&info->seed)) / (double)RAND_MAX;
        
        if (sqrt(xCoord * xCoord + yCoord * yCoord) <= 1.0)
            insideTheCircleCount++;
    }
    
    // pi/4 = inner/total
    *result = 4.0 * (double)insideTheCircleCount / (double)info->samplesCount; 

    return result;
}