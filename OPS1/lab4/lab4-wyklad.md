man 7 aio
man 3p aio_read
man 3p aio_write
man 3p aio_fsync
man 3p aio_error
man 3p aio_return
man 3p aio_suspend
man 3p aio_cancel
AIO wymaga librt podczas linkowania !!!

If we want to allow multiplatform ability we should
provide using ifndef and endif directives different implementaions 
based on the platform since AIO is not 100% portable between
POSIX systems.

```c
// error throws compilation error with the given message
#ifndef _POSIX_ASYNCHRONOUS_IO
#error System does not support asynchronous I/O
#endif
```

## Introduction to AIO
AIO controll block:
```c
struct aiocb {
    int aio_fildes;
    off_t aio_offset;
    volatile void *aio_buf;
    size_t aio_nbytes;
    int aio_reqprio;
    struct sigevent aio_sigevent;
    int aio_lio_opcode;
    ...
};
```
Controll block says that this strucure is used to describe one operation.
Program shall create this structure and fill it basing on
its requirments. 


Structures below are used to define how program should be informed
about end of the operation.
```c
union sigval 
{
    int sival_int;
    void *sival_ptr;
};

// describes how the synchronization will be performed
// after end of the operation
struct sigevent 
{
    // SIGEV_NONE, SIGEV_SIGNAL, SIGEV_THREAD 
    // SIGEV_NONE is deafult
    // SIGEV_SIGNAL - notification by signal
    // SIGEV_THREAD - notification by creating a thread
    int sigev_notify;

    int sigev_signo;
    union sigval sigev_value;

    // if SIGEV_THREAD is defined, we have to provide
    // pointer for the main procedure of the thread (thread function)
    // it has slightly different structure than p_thread
    void (*sigev_notify_function) (union sigval);


    void *sigev_notify_attributes;
    pid_t sigev_notify_thread_id; 
};
```

Controll block Members:
1. There is no asynchronous 'open' function, desired file
should be opened using synchronous function (flags are also
important, read, write etc.) and then its 
file descriptor should be provided in 'aio_fildes'.
2. Member 'aio_offset' says where in the file bytes should be processed, 
flags used in 'open' are important.
3. Member 'aio_buf' - the operation buffer, if we want to e.g. read some data
from the file this buffer will be filled.
4. Member 'aio_nbytes' - the maximum size of the buffer to read or write
5. Member 'aio_reqprio' - if we have lot of asynchronous operations, then 
we can give prorites, the bigger priority we give the more CPU access
will be possible.
6. Member 'aio_sigevent' (requires aio_fsync to work)
    - SIGEV_NONE – no notification
    - SSIGEV_SIGNAL – signal is sent (sigev_signo)
    - SIGEV_THREAD – specified function is stared as separate **detached** (joinable 
    are not possible) thread: 'sigev_notify_function', 'sigev_notify_attributes'
7. Member 'aio_lio_opcode' - significant if we have more than one 
asynchronous IO operations. There is a function 'lio_listio' for multiple
operations, opcode specifies if it is read or write operation (LIO_READ, LIO_WRITE),
LIO_NOP is used to store unused blocks on the list

Warning: It is advised to clear aiocb structure before using it.
Warning: One aiocb strucutre cannot be used to schedule two or more aio operations.

AIO on linux is implemented using threads, not in kernel. Anyone can implement
similar library using for example pthread. 

Warning: EINTR is impossible to observe.

## Functions
### aio_read
```c
int aio_read(struct aiocb *aiocbp);
```
It schedules single IO read operation and returns immediately.
This function is equivalent to synchronous read.
Multiple read operations on the same file part are possible.


opcode is ignored


file position after aio operation is unspecified according to POSIX

### aio_write
```c
int aio_write(struct aiocb *aiocbp);
```
Starts IO write operation and returns immediately.
If O_APPEND is not set 'aiocbp->aio_offset' is interpreted if it's set 
data will be appended

Warning: Multiple write operations on the same file part will produce
undefined results.

Warning: not every time cummulating multiple write functions by predicting appropriate
offsets is possible, since some of these writes can fail.


opcode is ignored


file position after aio operation is unspecified according to POSIX


### aio_write
```c
int aio_error(const struct aiocb *aiocbp);
```
Determines the status of operation, can be called several times, be careful not to implement busy waiting. 


If this function returns EINPROGRESS, we can't touch files, since 
AIO is still unfinished.


If zero is returned function finished and we can obtain it's status.

Errno shall be returned if error occured (perror will not work properly, 
but strerror (from C library) can be used).

### aio_return
```c
ssize_t aio_return(struct aiocb *aiocbp);
```
Frees aiocb.

Returns what corresponding read/write fsync would return.

Operation wont be interrupted by signal (no EINTR).

**Can be called only once after aiocb structure was used, 
cannot be called as long as aio_error returnes EINPROGRESS**.

### How to do busy waiting (you shouldn't do that)
```c
// wait for completion (resource consuming)
while ((reval = aio_error(&myaiocb)) == EINPROGRESS);

// free the aiocb
retval = aio_return(&myaiocb);
```

### Suspend approach (easiest but may waste some CPU time)
```c
int aio_suspend(const struct aiocb * const list[], int nent, 
const struct timespec *timeout);
```
- list of aiocb to wait for (join)
- nent is a size of this list (array)
- timeout is optional

Function suspends execution until **at least one** from the list of submitted aiocb points finished operation. By timeout, a time of suspend can be 

If any of operations on the list is finished by the time of call the function returns 
immediately. If we wait in the loop using suspend, after every end we shall 
remove it from the list in opposite case we create busy waiting.

Ignores NULL in the list.

The same list as for lio_listio can be used.

POSIX does not specify if aiocb structures with aio_lio_opcode set to LIO_NOP are ignored.

Warning: Suspend can be interrupted by a signal.

The function below waits for operation using suspend. 

Warning: It doesn't work for programms with signals.

```c
void suspend(struct aiocb *aiocbs) {
    struct aiocb *aiolist[1];
    aiolist[0] = aiocbs;

    while (aio_suspend((const struct aiocb *const *) aiolist, 1, NULL) == -1) 
    {
        if (!work) return; /*signal received*/
        else if (errno == EINTR) continue;
            error("Error suspending");
    }
    
    if (aio_error(aiocbs) != 0)
        error("Async. I/O error");
    if ((status = aio_return(aiocbs)) == -1)
        error("Async. I/O return error");
}
```

### aio_fsync

```c
    int aio_fsync(int op, struct aiocb *aiocbp);
```
