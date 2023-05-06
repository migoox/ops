#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <errno.h>

#define PARENT_SLEEP_TIME 3
#define DEAFULT_CHILD_COUNT 5
#define ERR(source)                                                                                                    \
	(fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), \
     perror(source), kill(0, SIGKILL), \
     exit(EXIT_FAILURE))

void usage(const char *prog_name)
{
    printf("Usage: %s [-n]\n", prog_name);
}

int main(int argc, char *argv[]) 
{
    int opt = -1;
    int n = DEAFULT_CHILD_COUNT;

    // handling arguments
    while ((opt = getopt(argc, argv, "n:")) != -1) 
    {
        switch (opt) 
        {
        case 'n':
            // if an error ocurred during converting optarg, 0 shall be returned
            n = atoi(optarg);
            break;
        default:
            usage(argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    // loop responsible for forking children
    for (int i = 0; i < n; i++)
    {
        // fork returns pid of the child to the parent process, 0 to the child process
        // and -1 if an error occured
        pid_t pid = fork();
        if (pid == 0) 
        {
            // srand should be inside of the child, if it was in the parent
            // each child would have the same seed and rand() would return
            // the same value

            // time(NULL) can stand alone since all the forks happen in the same 
            // second so the seed would be the same for each child
            srand(time(NULL) * getpid());

            int seconds = 5 + rand() % (10 - 5 + 1);

            // sleep returns unslept time, usually if there was no signal delivered
            // it returns 0, that means that process slept for requested time
            sleep(seconds);

            // as this is a child process calling getpid in this scope will return 
            // child's pid
            printf("I am child with pid: %d\n", getpid());

            // without exiting the child, child will go out of the scope of if
            // and behave as its parent
            exit(EXIT_SUCCESS);
        } 
        
        if (fork() == -1)
        {
            ERR("Fork failed");
        }
    }

    // loop responsible for wating children
    while (n > 0)
    {
        sleep(PARENT_SLEEP_TIME);

        // the loop is infinite since we don't know how many
        // children are ready to be waited by the parent
        while (1)
        {
            errno = 0;
            // input:
            //  - stat_loc = NULL means that we dont want to know about status
            //  - options = WNOHANG means that parent will pause until the child
            // will terminate, instead if there are not terminated children waitpid 
            // shall return 0
            pid_t pid = waitpid(0, NULL, WNOHANG);

            // pid is greater than 0 => the child has been waited by the parent
            // and pid variable contains pid of the waited child
            if (pid > 0)
            {
                n--;
            }
            // if pid is 0 (only happens with WNOHANG flag raisen) => there is at 
            // least one child process but it's not terminated (status is not available)
            else if (pid == 0)
            {
                break;
            }
            else // pid < 0
            {
                // if the calling process has no existing unwaited-for child processes
                // errno flag will be set to ECHILD, this will happen if 
                // all of the children are waited
                if (errno == ECHILD)
                    break;
                
                // in this place of the program we can be sure that there is an error
                ERR("waitpid() error");            
            }
            printf("I am parent with %d children\n", n);
        }
    }

    return EXIT_SUCCESS;
}
