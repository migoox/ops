| Function | Description |
|----------|-------------|
| [`pthread_create`](#pthread_create) | Create a new thread |
| [`pthread_join`](#pthread_join) | Wait for a thread to terminate |
| [`pthread_mutex_init`](#pthread_mutex_init) | Initialize a mutex |
| [`pthread_mutex_destroy`](#pthread_mutex_init) | Destroy a mutex |
| [`pthread_mutex_lock`](#pthread_mutex_lock) | Lock a mutex |
| [`pthread_mutex_unlock`](#pthread_mutex_lock) | Unlock a mutex |
| [`pthread_mutex_trylock`](#pthread_mutex_lock) | Try to lock a mutex |
| [`pthread_detach`](#pthread_detach) | Mark a thread as detached |
| [`pthread_attr_init`](#pthread_attr_destroy) | Initialize a thread attribute object |
| [`pthread_attr_destroy`](#pthread_attr_destroy) | Destroy a thread attribute object |
| [`pthread_attr_getdetachstate`](#pthread_attr_getdetachstate) | Get the detached state of a thread attribute object |
| [`pthread_attr_setdetachstate`](#pthread_attr_getdetachstate) | Set the detached state of a thread attribute object |
| [`pthread_sigmask`](#pthread_sigmask) | Set the signal mask for a thread |
| [`sigwait`](#sigwait) | Wait for a signal |
| [`pthread_cancel`](#pthread_cancel) | Cancel a thread |
| [`pthread_setcancelstate`](#pthread_setcancelstate) | Set the cancellation state for a thread |
| [`pthread_setcanceltype`](#pthread_setcanceltype) | Set the cancellation type for a thread |
| [`pthread_cleanup_push`](#pthread_cleanup_push) | Push a cleanup handler onto the cleanup stack/Pop a cleanup handler from the cleanup stack |
| [`getres`](#getres) | Get the resolution of a timer |
| [`rand_r`](#rand_r) | Generate a random number using a seed value |

## Tools
```c
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

#define ELAPSED(start, end) ((end).tv_sec - (start).tv_sec) + (((end).tv_nsec - (start).tv_nsec) * 1.0e-9)
#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))

typedef struct timespec timespec_t;
typedef unsigned int UINT;

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
```

```Makefile
CC=gcc
CFLAGS=-std=gnu99 -Wall -fsanitize=address,undefined
LDFLAGS=-fsanitize=address,undefined
LDLIBS=-lpthread -lm
```

## Theory
Multithreading is a technique where a process is divided into multiple threads of execution that can run concurrently (or parralel to each other). **Each thread shares the same memory space (registers and stack) and can access shared resources** (forking creates new process with its own memory space), but has its own independent flow of control. This allows multiple tasks to be executed in parallel within the same process, making it more efficient and responsive.

**Concurrent execution (współbieżność)** - single core, tasks are not running at the 
same time, but from the user point of view it feels like that, bus of some units of 
multiple tasks.
**Concurrent execution (współbieżność)** - multi-core systems, implies that a system can
perform more than one task simultaneously.

**User threads** are implemented and managed entirely within the user space, while **kernel threads** are implemented and managed within the kernel. User threads are lightweight and easy to create, but they are limited by the resources of the user space. Kernel threads are more powerful and flexible, but also more complex and resource-intensive.

User threads (libraries) use kernel threads, there are 3 models of dealing with that.

**Multithreading Models:**
- *Many-to-One*
- *One-to-One*
- *Many-to-Many*  

### Synchronous vs asynchronous multithreading
**Synchronous multithreading**, refers to a programming model in which threads are executed in a synchronous manner, meaning that they are executed in a predictable order and at predetermined times. In a synchronous multithreaded system, threads are executed in a predetermined sequence, and their execution is not interrupted or interleaved.

**Asynchronous multithreading** is often used in systems that need to handle a high volume of concurrent tasks or that need to respond to external events or stimuli. It can improve the performance and responsiveness of a system by allowing tasks to be executed concurrently and by allowing tasks to make progress even if some tasks are blocked or waiting for resources.


### Pthreads library 
**Manpages:** man 7 pthreads
A single process can contain multiple threads, all of which are executing the same  program. These  threads  share  the same global memory (data and heap segments), but each thread has its own stack (automatic variables). 

POSIX requires that threads share a range of attributes, these include:
- process ID
- parent process ID
- process group ID and session ID
- controlling terminal
- user and group IDs
- open file descriptors
- file mode creation mask
- current directory (**chdir**) and root directory (**chroot**)
- thread ID
- signal mask
- errno variable
- alternate signal stack

### Pthread Functions and errors
Most Pthread functions return 0 on success and, an error number on failure (since the errno value is process-wide). The error number has the same meaning that errno value has, so **perror** can be used to deal with them. 

For each of the pthreads functions that can
return an error, specifies that the function can never fail with an error **EINTR**.

### Thread IDs
Each of the threads in a process has a unique thread identifier (type **pthread_t**). This identifier is returned   to  the  caller  of  **pthread_create**,  and  a  thread  can  obtain  its  own  thread  identifier  using **pthread_self**. 

Thread IDs are guaranteed to be unique only within a process and in contrast with pid's they are not incrementing.

The  system  may  reuse  a thread ID after a terminated thread has been joined, or a detached thread has terminated (undefined behaviour).

### Thread-safe function
A thread-safe function is one that can be safely (i.e., it will deliver the same results regardless of whether it is) called from multiple threads at the same time. List of thread-safe functions can be found on **man 7 pthreads**.

Note that sleep is not thrad-safe, but nanosleep is.

### Cancelation points
In the pthread library, cancellation is a mechanism that allows one thread to terminate another thread.

POSIX specifies that certain functions must, and certain other functions may, be cancellation points. If a thread is cancelable, its cancelability type is deferred, and a cancellation request is pending for the thread, then the thread is canceled when it calls a function that is a cancellation point.

List of functions that are cancellation points can be found on **man 7 pthreads**.

### Time
**Real  time**  is defined as time measured from some fixed point, either from a standard point in the past, or from some point (e.g., the start)  in  the  life  of  a  process(elapsed time).

**Process time** is defined as the amount of CPU time used by a process. This is sometimes divided into user and system components.
- User CPU time is the time spent executing code in user mode. 
- System CPU time is the time spent by the kernel executing in system mode on behalf of the process

**Hardware clock** battery-powered which the kernel reads at boot time in order to initialize the software clock.

The accuracy of various system calls that set timeouts and  measure  CPU  time is limited by the resolution of the **software clock**, a clock maintained by the kernel which measures time in **jiffies**. The size of a jiffy is determined by the value of the kernel constant **HZ**.

ince   Linux   2.6.21,   Linux   supports   high-resolution   timers   (HRTs),  optionally  configurable  via  CONFIG_HIGH_RES_TIMERS. On a system that supports HRTs, the accuracy of sleep and timer system calls is no longer constrained  by  the  jiffy,  but instead can be as accurate as the hardware allows. You can determine whether high-resolution timers are supported by  checking  the  resolution  returned by a call to **clock_getres(2)** or looking at the "resolution" entries in /proc/timer_list.
## pthread_create
**Manpages:** man 3p pthread_create
```c
#include <pthread.h>
int pthread_create(pthread_t *restrict thread,
           const pthread_attr_t *restrict attr,
           void *(*start_routine)(void*), void *restrict arg);
```
### Description 
function  shall  create a new thread, within a process. The signal state of the new thread shall be initialized as follows:
- The signal mask shall be inherited from the creating thread.
- The set of signals pending for the new thread shall be empty.

The thread-local current locale and the alternate stack shall not be inherited. The floating-point environment shall be inherited from the creating thread.

If  pthread_create() fails, no new thread is created and the contents of the location referenced by thread are undefined.

**Input-values:**
- pthread_t *restrict thread  - returns TID if succeded.
- const pthread_attr_t *restrict attr - pointer to thread attributes structure, if it's NULL, the default attributes shall be used. If attr is modified later, thread's  attributes  shall not be affected. 
- void \*(\*start_routine)(void*) - working function. If the start_routine returns,  the  effect shall be as if there was an implicit call to pthread_exit()
- void *restrict arg - working function argument, it is void, so it can be a pointer
to a structure

###Return value If successful, the pthread_create() function shall return zero; otherwise, an error number shall be returned to indicate the error.

## pthread_join
```c
#include <pthread.h>
int pthread_join(pthread_t thread, void **value_ptr);
```
### Description 
function shall suspend execution of the calling thread until the target thread terminates, unless the target thread already terminated.

**Input-values:**
- pthread_t thread - TID of the thread 
- void **value_ptr - the value passed to pthread_exit() by the terminating thread shall be made available in the location referenced by value_ptr. When a pthread_join() returns successfully, the target thread has been  terminated.

### Return value 
If successful, the pthread_join() function shall return zero; therwise, an error number shall be returned to  indicate the error.

## pthread_mutex_init
**Manpages:** man 3p pthread_mutex_destroy
```c
#include <pthread.h>
int pthread_mutex_destroy(pthread_mutex_t *mutex);
int pthread_mutex_init(pthread_mutex_t *restrict mutex,
    const pthread_mutexattr_t *restrict attr);
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
```

### Description 

The pthread_mutex_init() function shall initialize the <ins>mutex</ins> referenced by mutex with attributes specified by <ins>attr</ins>. 

The  pthread_mutex_destroy()  function shall destroy the mutex object referenced by mutex; the mutex object becomes, in effect, uninitialized.

A  destroyed  mutex object can be reinitialized using pthread_mutex_init().

Attempting to initialize an already initialized mutex results in undefined behavior.

The macro **PTHREAD_MUTEX_INITIALIZER** can be used to initialize mutexes. The effect shall be equivalent to dynamic initialization by a call to **pthread_mutex_init** with parameter <ins>attr</ins> specified as NULL, except that no error checks are performed.

| Mutex Type | Description |
|------------|-------------|
| PTHREAD_MUTEX_DEFAULT | The default mutex type. This type of mutex has undefined behavior when used with pthread_cond_timedwait or pthread_cond_wait. |
| PTHREAD_MUTEX_NORMAL | A "normal" mutex that is not prone to deadlock. This type of mutex may not be used with pthread_cond_timedwait or pthread_cond_wait. |
| PTHREAD_MUTEX_ERRORCHECK | A mutex that provides error checking. If a thread attempts to unlock a mutex that it has not locked, or to unlock a mutex that is already unlocked, an error will be returned. |
| PTHREAD_MUTEX_RECURSIVE | A mutex that allows a thread to lock the same mutex multiple times without causing a deadlock. When a thread has locked a mutex for the nth time, it must also unlock the mutex n times before another thread can successfully lock it. |


### Return value 

If successful, the pthread_mutex_destroy() and pthread_mutex_init() functions shall return zero; otherwise, an error number shall be returned to indicate the error.

### Example
```c
// PTHREAD_MUTEX_DEFAULT mutex
pthread_mutex_t mutex1;
int ret = pthread_mutex_init(&mutex1, PTHREAD_MUTEX_DEFAULT);
if (ret != 0) {
    // handle error
}

// PTHREAD_MUTEX_DEFAULT mutex
pthread_mutex_t mutex2;
ret = pthread_mutex_init(&mutex2, NULL);
if (ret != 0) {
    // handle error
}

// PTHREAD_MUTEX_DEFAULT mutex with no error checking
pthread_mutex_t mutex3 = PTHREAD_MUTEX_INITIALIZER;
```

## pthread_mutex_lock
**Manpages:** man 3p pthread_mutex_lock
```c
#include <pthread.h>
int pthread_mutex_lock(pthread_mutex_t *mutex);
int pthread_mutex_trylock(pthread_mutex_t *mutex);
int pthread_mutex_unlock(pthread_mutex_t *mutex);
```

### Description 

The  mutex  object referenced by mutex shall be locked by a call to **pthread_mutex_lock()** that returns zero or \[EOWNERDEAD\]. 

If the mutex is already locked by another thread, the calling thread shall block until the  mutex  becomes available. This operation shall return with the mutex object referenced by mutex in the locked state with the calling thread as its owner.

The  **pthread_mutex_trylock()**  returns immediately with an error if the mutex is already locked.

The **pthread_mutex_unlock()** function shall release the mutex object referenced by mutex.
The manner in which a mutex is released is dependent upon the mutex's type attribute.

### Return value 

If successful, the pthread_mutex_lock(), pthread_mutex_trylock(), and pthread_mutex_unlock() functions shall return zero; otherwise, an error number shall be returned to indicate the error.

## pthread_detach
**Manpage:** man 3p pthread_detach
```c
#include <pthread.h>
int pthread_detach(pthread_t thread);
```
### Description 

function is used to mark a thread as detached. When a thread is marked as detached, it will automatically release all of its resources (such as memory) when it terminates, without the need for another thread to join with it. If <ins>thread</ins> has not terminated, **pthread_detach()** shall not cause it to terminate.

It's important to note that once a thread is detached, it is not possible to join with it again, and any resources or memory allocated by the thread will be automatically released when it terminates.

### Return value 

If  the call succeeds, pthread_detach() shall return 0; otherwise, an error number shall be returned to indicate the error.

### Example
```c
#include <pthread.h>

void *thread_func(void *arg) {
    // Do some work here
    // ...

    return NULL;
}

int main() {
    pthread_t thread;
    int ret;

    // Create thread
    ret = pthread_create(&thread, NULL, thread_func, NULL);
    if (ret != 0) {
        // handle error
    }

    // Detach thread
    ret = pthread_detach(thread);
    if (ret != 0) {
        // handle error
    }

    // Do some work in main thread
    // ...

    return 0;
}
```

## pthread_attr_destroy
**Manpage:** man 3p pthread_attr_destroy
```c
#include <pthread.h>
int pthread_attr_destroy(pthread_attr_t *attr);
int pthread_attr_init(pthread_attr_t *attr);
```

### Description 
The  **pthread_attr_init()**  function  shall  initialize the thread attributes object.
The  **pthread_attr_destroy()**  function  shall  destroy  a  thread  attributes  object.

### Return value 

Upon  successful completion, **pthread_attr_destroy()** and **pthread_attr_init()** shall return a value of 0; otherwise, an error number shall be returned to indicate the error.

Structure **pthread_attr_t** has two members - detachstate and scope, but these
can be retrieved/changed only by using functions:
- **pthread_attr_setXXX(pthread_attr_t *attr, int value)**,
- **pthread_attr_getXXX(pthread_attr_t *attr, int *value)**,
where XXX can be replaced with either "detachstate" or "scope".

## pthread_attr_getdetachstate
**Manpage:** man 3p pthread_attr_getdetachstate
```c
#include <pthread.h>
int pthread_attr_getdetachstate(const pthread_attr_t *attr, int *detachstate);
int pthread_attr_setdetachstate(pthread_attr_t *attr, int detachstate);
```

### Description 

The  <ins>detachstate</ins> attribute controls whether the thread is created in a detached state. 

If the thread is created detached, then use of the ID of the newly created thread by the **pthread_detach()** or **pthread_join()** function is an  error.

A value of PTHREAD_CREATE_DETACHED shall cause all threads created with attr to be in the  detached  state,  whereas using  a value of PTHREAD_CREATE_JOINABLE shall cause all threads created with attr to be in the joinable state. The default value of the detachstate attribute shall be PTHREAD_CREATE_JOINABLE. 

### Return value 

Upon  successful completion, **pthread_attr_getdetachstate()** and **pthread_attr_setdetachstate()** shall return a value of 0; otherwise, an error number shall be returned to indicate the error.

### Example

```c
pthread_attr_t attr;
int detachstate;
int ret = pthread_attr_getdetachstate(&attr, &detachstate);
if (ret != 0) {
    // handle error
}

if (detachstate == PTHREAD_CREATE_DETACHED)
{
    // do something
}
```

```c
pthread_attr_t attr;
int ret = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
if (ret != 0) {
    // handle error
}
```

## pthread_sigmask
**Manpage:** man 3p pthread_sigmask
```c
#include <signal.h>
int pthread_sigmask(int how, const sigset_t *restrict set,
    sigset_t *restrict oset);
int sigprocmask(int how, const sigset_t *restrict set,
    sigset_t *restrict oset);
```

### Description 

A signal mask is a set of signals that are currently blocked from delivery to the thread. When a signal is blocked, it means that the signal is not delivered to the thread until it is unblocked.

**pthread_sigmask** is used to manipulate the signal mask of the calling thread, while **sigprocmask** is used to manipulate the signal mask of the calling process, which means that the call will affect all threads in the process, not just the calling thread.

**Input values:** 

The pthread_sigmask function takes two arguments:
- how: This argument specifies how the signal mask should be modified. It can be one of the following values:
    - SIG_BLOCK: The signals specified in the second argument are added to the current signal mask.
    - SIG_UNBLOCK: The signals specified in the second argument are removed from the current signal mask.
    - SIG_SETMASK: The signal mask is set to the signals specified in the second argument.
- set: This argument is a pointer to a sigset_t data structure that specifies the signals to be added to, removed from, or set in the signal mask.

The pthread_sigmask function returns 0 on success and a non-zero error code on failure.

### Return value 

Upon successful completion pthread_sigmask() shall return 0; otherwise, it shall return the corresponding error number. 

Upon successful completion, sigprocmask() shall return 0; otherwise, -1 shall be returned, errno shall be set.

```c
#include <pthread.h>
#include <signal.h>

void* thread_func(void* arg)
{
    sigset_t sigset;
    sigemptyset(&sigset);
    // sigfillset(&sigset); - blocks all signals that can be blocked
    sigaddset(&sigset, SIGINT);
    pthread_sigmask(SIG_BLOCK, &sigset, NULL);

    // Code that runs in the thread goes here

    return NULL;
}

int main()
{
    pthread_t thread;
    pthread_create(&thread, NULL, thread_func, NULL);
    pthread_join(thread, NULL);
    return 0;
}
```

## sigwait
**Manpage:** man 3p sigwait
```c
#include <signal.h>
int sigwait(const sigset_t *restrict set, int *restrict sig);
```
### Description 

Allows a thread to wait for a specific signal to be delivered. The sigwait function blocks the calling thread until one of the signals specified in the set argument is delivered to the thread.

**Differences between sigsuspend and sigwait:**
- **sigwait** takes a sigset_t data structure as an argument, which specifies the set of signals that the thread is waiting for, while **sigsuspend** takes a signal mask as an argument, which specifies the signals that are blocked while the thread is suspended
- **sigwait** blocks the calling thread until one of the specified signals is delivered, while **sigsuspend** temporarily replaces the current signal mask with the specified signal mask and then blocks the calling thread until a signal is delivered

**Input values:** 

The sigwait function takes two arguments:
- set: This argument is a pointer to a sigset_t data structure that specifies the set of signals that the thread is waiting for.
- sig: This argument is a pointer to an int variable that will be set to the signal number of the signal that was delivered.

### Return value 
Upon successful completion, sigwait() shall store the signal number of the received signal at  the  location  referenced by sig and return zero. Otherwise, an error number shall be returned to indicate the error.

### Example

```c
#include <pthread.h>
#include <signal.h>
#include <stdio.h>

void* thread_func(void* arg)
{
    sigset_t sigset;
    sigemptyset(&sigset);
    sigaddset(&sigset, SIGINT);
    pthread_sigmask(SIG_BLOCK, &sigset, NULL);

    int sig;
    // wait for SIGINT
    int ret = sigwait(&sigset, &sig);
    if (ret == 0) {
        printf("Received signal %d\n", sig); // Received signal SIGINT
    } else {
        printf("sigwait failed with error %d\n", ret);
    }
    return NULL;
}

int main()
{
    pthread_t thread;
    pthread_create(&thread, NULL, thread_func, NULL);
    pthread_join(thread, NULL);
    return 0;
}
```

## pthread_cancel
**Manpage:** man 3p pthread_cancel
```c
#include <pthread.h>
int pthread_cancel(pthread_t thread);
```
### Description

Function is used to cancel a thread in a multithreaded program. When a thread is canceled, it is terminated immediately, and any resources or memory allocated by the thread are released.

Canceling a thread is different from terminating a thread normally, as the thread is not given an opportunity to clean up or release any resources it has allocated. 

The target thread's cancelability state and type determines when the cancellation takes effect. 

When the cancellation is acted on, the cancellation cleanup handlers  for  thread shall be called. 

When the last cancellation cleanup handler returns, the thread-specific data destructor functions shall be called for thread. When the last destructor function returns, thread  shall  be  terminated.

## pthread_setcancelstate, pthread_setcanceltype
**Manpages:** man 3p pthread_setcancelstate
```c
#include <pthread.h>
int pthread_setcancelstate(int state, int *oldstate);
int pthread_setcanceltype(int type, int *oldtype);
```

### Description
There are two parameters that can be chagned:
- cancelability (set set to PTHREAD_CANCEL_ENABLE, by default), can be changed to PTHREAD_CANCEL_ DISABLE. In effect cancellation requests are held pending.
- cancelability type (set to PTHREAD_CANCEL_ DEFERRED in all newly
created threads). 
- When cancelability is enabled and the cancelability type is
PTHREAD_CANCEL_ DEFERRED cancellation requests are held
pending until a cancellation point is reached (A cancellation point is a point in the execution of a thread where the thread checks for cancellation and exits if it has been canceled.)
- When cancelability is enabled and the cancelability type is
PTHREAD_CANCEL_ ASYNCHRONOUS, new or pending cancellation
requests may be acted upon at any time. Only async-cancel-safe
functions should be used by a thread with asynchronous cancelability.

## pthread_cleanup_push
**Manpage:** man 3 pthread_cleanup_push
```c
#include <pthread.h>
void pthread_cleanup_push(void (*routine)(void *), void *arg);
void pthread_cleanup_pop(int execute);
```

### Description

Functions provide a mechanism for defining cleanup routines that are executed when a thread is canceled or exits.

The **pthread_cleanup_push()** function is used to register a cleanup routine that will be called when the thread is canceled or exits. 

It takes two arguments: a pointer to a function that will be called as the cleanup routine, and an argument that will be passed to the cleanup routine when it is called. 

The cleanup routine should have the following prototype:
```c
void cleanup_routine(void *arg);
```
The **pthread_cleanup_pop()** function is used to remove the most recently registered cleanup routine from the thread's cleanup stack. 

It takes a single argument that specifies whether the cleanup routine should be called before it is removed. If the argument is non-zero, the cleanup routine will be called; if it is zero, the cleanup routine will not be called.

**Example:**
```c
void *thread_func(void *arg)
{
    int sockfd = *((int *)arg);

    pthread_cleanup_push(close_socket, &sockfd);
    // Thread code goes here
    pthread_cleanup_pop(1);

    return NULL;
}
```

## getres
**Manpages:** man 3p getres
```c
#include <time.h>
int clock_getres(clockid_t clock_id, struct timespec *res);
int clock_gettime(clockid_t clock_id, struct timespec *tp);
int clock_settime(clockid_t clock_id, const struct timespec *tp);
```
### Description 

The  clock_getres()  function shall return the resolution of any clock. Clock resolutions are implementation-defined and cannot be set by a process.

If the argument res is not NULL, the resolution of  the  specified  clock  shall  be
stored  in  the location pointed to by res.

The clock_gettime() function shall return the current value tp for the specified clock, clock_id.

The clock_settime() function shall set the specified clock, clock_id, to the value specified  by  tp. Time  values that  are  between  two consecutive non-negative integer multiples of the resolution of the specified clock shall be truncated down to the smaller multiple of the resolution.

### Example
```c
timespec_t start, end;
if (clock_gettime(CLOCK_REALTIME, &start))
    ERR("clock_gettime() failed");

while (1)
{
    // do something

    if (clock_gettime(CLOCK_REALTIME, &end))
        ERR("clock_gettime() failed");

    // break the loop if time have elapsed
    if (ELAPSED(start, end) >= 4.0)
        break;
}
```

## rand_r
**Manpages:** man 3p rand
```c
#include <stdlib.h>
int rand(void);
int rand_r(unsigned *seed);
void srand(unsigned seed);
```
### Description 
the rand_r() is thread safe in contrast with rand(), as it does not use a global seed value and allows each thread to have its own seed value.