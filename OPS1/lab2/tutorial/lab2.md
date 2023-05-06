# Lab 2
| System Call | Description |
|-------------|-------------|
| [`fork()`](#fork) | This system call creates a new child process that is identical to the calling process. |
| [`getpid()`](#getpid) | This system call returns the process ID of the calling process. |
| [`wait()`](#wait) | This system call causes the calling process to wait for any of its child processes to terminate. |
| [`waitpid()`](#waitpid) | This system call causes the calling process to wait for a specific child process to change state. |
| [`sleep()`](#sleep) | This system call causes the calling process to sleep for a specified number of seconds. |
## Usage function and ERR macro
```c
#define ERR(source) \
	(fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), \
     perror(source), kill(0, SIGKILL), \
     exit(EXIT_FAILURE))

void usage(const char *prog_name)
{
    printf("Usage: %s [-n]\n", prog_name);
}
```

## fork
**Manpage:** man 3p fork
```c
#include <unistd.h>

pid_t fork(void);
```
**Description:** creates a new child process, which is an exact copy of its parent with new memory space. Important differences between child process and its parent:
- the child process has a unique process ID
- the child process has a different parent process ID than its parent
- the child process ID shall not match any active process group ID 
- the child has its own copies of file descriptors (small integers that are used to identify a file or other input/output resource) used by the parent

**Return value:**

Upon successful completion:

- fork() shall return **0** to the child process
- fork() shall return **pid > 0** of created child to the parent process

Otherwise:
- fork() shall return **-1** to the parent process and errno shall be set to indicate an error

## getpid
**Manpage:** man 3p getpid
```c
#include <unistd.h>

pid_t getpid(void);
```
**Description:** get the process ID of calling process
**Return value:** getpid() should always be successful, no return value is reserved to indicate an error

## wait
**Manpage:** man 3p wait
```c
#include <sys/wait.h>

pid_t wait(int *stat_loc);
```
**Input values:**

`int *stat_loc` - reference to the integer where obtained status information shall be placed, if it's NULL, information won't be return

**Description:** wait() should obtain status information for process termination from any child process. 

If there is no status information and at least one child is running, wait() will cause the calling process to pause and wait until one of the children will terminate. After obtaining information for process termination the calling process will be unpaused.

If there are more than one terminated child's status information waiting for being obtain, the process termination is choosen in undefined way.

**Return value:**

If succeded:
- returns a **pid of the child** process for which the status is reported

Otherwise:
- returns **-1** and sets errno flag

**Errors:**
- ECHILD - the calling process has no existing unwaited-for child processes
- EINTR - the function was interrupted by a signal, the value stat_loc is undefined

## waitpid
**Manpage:** man 3p wait
```c
#include <sys/wait.h>

pid_t waitpid(pid_t pid, int *stat_loc, int options);
```

**Input values:**

- `pid_t pid` - specifies a set of child processes for which status is requested:
    - *pid = (pid_t) - 1* - behaves in the same way as wait(), status requested for any child process
    - *pid > 0* - specifies the process ID of a single child process for which status is requested
    - *pid = 0* - status is requested for any child process whose process group ID is equal to that of the calling process
    - *pid < (pid_t) - 1* - status is requested for any child process whose process group ID is equal to the absolute value of pid
- `int *stat_loc` - reference to the integer where obtained status information shall be placed, if it's NULL, information won't be return
- `int options` - constructed from the bitwise-inclusive OR of zero or more of the following flags
    - *WNOHANG* - if there are no terminated children, the calling process won't be paused in order to wait for one, the program will continue
    - *WUNTRACED* - causes function to return when a child process is stopped
    - *WCONTINUED* - causes function to return when a child process is continued after being stopped

**Return value:**
If succeded:
- returns a **pid of the child** process for which the status is reported

Otherwise:
- returns **-1** and sets errno flag

If *WNOHANG* flag is raised:
- **0** can be returned if there are running children but none of them is terminated

**Errors:**
- ECHILD - the calling process has no existing unwaited-for child processes
- EINTR - the function was interrupted by a signal, the value stat_loc is undefined

## sleep
**Manpage:** man 3p sleep
```c
#include <unistd.h>

unsigned sleep(unsigned seconds);
```
**Description:**

The  sleep() function shall cause the calling thread to be suspended from execution until either the number of real‐time seconds specified by the argument seconds has elapsed or a signal is delivered to the calling  thread  and  its action  is  to invoke a signal-catching function or to terminate the process.

**Return value:**

sleep returns unslept amount of time, which is different than **0** if a signal has been sent during sleeping.

## Signals
**Manpage:** man 3p signals

Linux supports both POSIX reliable signals (hereinafter "standard signals") and POSIX real-time signals.

Each signal has a current disposition, which determines how the process behaves when it is delivered the signal.

| Signal       | Default Action |
|--------------|----------------|
| `Term`    | Terminate the process |
| `Ign`     | Ignore the signal |
| `Core`    | Terminate the process and dump core |
| `Stop`    | Stop the process |
| `Cont`    | Continue the process if it is currently stopped |

A process can change the disposition of a signal using sigaction(2) or signal(2).

Using these system calls, a process can elect one of the following  behaviors to occur on delivery of the signal: 
- perform the default action; ignore the signal; 
- or catch the signal with a signal handler, a programmer-defined function that is automatically invoked when the signal is  delivered.

A  child created via fork(2) inherits a copy of its parent's signal dispositions.  During an execve(2), the dispositions of handled signals are reset to the default.

**Signal mask and pending signals**

A signal may be blocked, which means that it will not be delivered until it is later unblocked. Between  the  time when it is generated and when it is delivered a signal is said to be pending.

Each  thread in a process has an independent signal mask, which indicates the set of signals that the thread is currently blocking.

A  child  created  via  fork(2) inherits a copy of its parent's signal mask; the signal mask is preserved across execve(2).

## sigaction
**Manpage:** man 3p sigaction
```c
#include <signal.h>

int sigaction(int sig, const struct sigaction *restrict act,
    struct sigaction *restrict oact);
```

**sigaction structure**
```c
struct sigaction
{
    // Pointer to a signal-catching function or
    // one of the macros SIG_IGN or SIG_DFL.
    void(*) (int) sa_handler;

    // Additional set of signals to be blocked during
    // execution of signal-catching function.
    sigset_t mask;

    // Special flags to affect behavior of signal.
    int sa_flags;

    // Pointer to a signal-catching function. 
    void(*) (int, siginfo_t *, void *) sa_sigaction;

    // Application shouldn't use both - sa_handler and 
    // sa_sigaction, since the storage occupied by 
    // both functions may overlap.
}
```

**Input values:**

- 'int sig' - specyfies the signal, eg. *SIGUSR1*
- 'const struct sigaction *restrict act' - if it's not a NULL, it points to the structure specifying the action to be associated with the specified signal.
- 'struct sigaction *restrict oact' - if it's not a NULL, the action previously associated with the signal is stored here

**Return value:**

Upon successful completion, sigaction() shall return 0; otherwise, -1 shall be returned, errno shall be set to indicate the error, and no new signal-catching function shall be installed.

**Example:**
```c
void set_handler(int signal, void (*handler)(int))
{
    struct sigaction sa;

    // initalizing all of the members with zero
    memset(&sa, 0, sizeof(struct sigaction));

    // an alternative way:
    //      sigemptyset(&sa.sa_mask);
    //      sa.sa_flags = 0;

    // setting handler 
    sa.sa_handler = handler;

    if (sigaction(signal, &sa, NULL) == -1)
    {
        ERR("sigaction() failed");
    }
}
```
## nanosleep
**Manpage:** man 3p nanosleep
```c
#include <time.h>

int nanosleep(const struct timespec *rqtp, struct timespec *rmtp);
```

**Input values:**
- `rqtp`: A pointer to a struct timespec that specifies the amount of time the calling process should sleep. The struct has the following two fields:
    - `tv_sec`: The number of seconds to sleep.
    - `tv_nsec`: The number of nanoseconds to sleep, in addition to  `tv_sec` field.
- `rmtp`: An optional pointer to a struct timespec where the remaining time will be stored if the sleep is interrupted by a signal.


**Return value:**
- If the nanosleep() function returns because the requested time has elapsed, its return value shall be **0**.
- If the nanosleep() function returns because it has been interrupted by a signal, it shall return a value of **-1** and set errno to indicate the interruption. If `rmtp` isn't NULL it will conain unslept time

## alarm
**Manpage:** man 3p alarm
```c
#include <unistd.h>

unsigned alarm(unsigned seconds);
```

**Input values:**
- `seconds`: The number of seconds to wait before sending the signal.

**Description:** The  alarm()  function shall cause the system to generate a SIGALRM (Term) signal for the process after the number of real-time seconds specified by seconds have elapsed. Processor scheduling delays may prevent the process  from  handling the signal as soon as it is generated.

**Return value:**
If there is a previous alarm() request with time remaining, alarm() shall return a non-zero value that is the number of seconds until the previous request would have generated a SIGALRM signal. Otherwise, alarm() shall return 0.

**Warning:** Pamiętaj o możliwym KONFLIKCIE sleep i alarm - wg. POSIX sleep może używać w implementacji SIGALRM a nie ma jak zagnieżdżać sygnałów, nigdy zatem w kodzie oczekującym na alarm nie używamy sleep, można za to użyć nanosleep tak jak w kodzie powyżej.
## memset
```c
#include <string.h>

void *memset(void *s, int c, size_t n);
```

**Description:** The  memset() function  shall  copy  c (converted to an unsigned char) into each of the first n bytes of the object pointed to by s.

**Return value:** The memset() function shall return s; no return value is reserved to indicate an error.

## kill
**Manpages:** man 3p kill
```c
#include <signal.h>

int kill(pid_t pid, int sig);
```
The  kill()  function  shall  send a signal to a process or a group of processes specified by pid. If sig is 0  (the  null  signal), error checking  is  performed  but no signal is actually sent. The null signal can be used to check the validity of pid.

**Input value:**

- `pid > 0`: `sig` shall be sent to the process whose process ID is equal to pid
- `pid = 0`: `sig` shall be sent to all processes (excluding an unspecified set of  system  processes) whose process group ID is equal to the process group ID of the sender
- `pid = -1`: `sig` shall be sent to all processes (excluding an unspecified set of system processes) for which the process has permission to send that signal
- `pid < 0 && pid != -1`: `sig` shall be sent to all processes(excluding an unspecified set of system  processes)  whose  process  group ID is equal to the absolute value of pid, and for which the process has permission to end a signal.

**Return value:**
Upon successful completion, 0 shall be returned. Otherwise, -1 shall be returned and errno set to indicate  the  error.

## sigsuspend
**Manpage** man 3p sigsuspend
```c
#include <signal.h>

int sigsuspend(const sigset_t *sigmask);
```
**Description:** Temporarily replaces the signal mask of the calling process with a given set of signals and then waits for a signal to be delivered. When a signal is delivered, sigsuspend() restores the signal mask and returns, allowing the process to continue executing. sigsuspend() is typically used in a multi-threaded program to atomically wait for a signal and temporarily block all other signals.

**Example:**
```c
#include <signal.h>
#include <unistd.h>
#include <errno.h>

// Set up a signal mask that blocks SIGINT and SIGTERM
sigset_t mask;
sigemptyset(&mask);
sigaddset(&mask, SIGINT);
sigaddset(&mask, SIGTERM);

// Atomically wait for a signal and temporarily block all other signals
if (sigsuspend(&mask) == -1) {
    // Handle error
    if (errno == EINTR) {
        // The call was interrupted by a signal
        // (this is not considered an error)
    } else {
        // Some other error occurred
    }
}

// Do something else once the signal has been received

```

**Return value:**

- The sigsuspend() function returns -1 if an error occurs and sets the global errno variable to indicate the error. 

- Otherwise, it does not return and instead waits for a signal to be delivered. When a signal is delivered, sigsuspend() restores the signal mask and returns, allowing the process to continue executing.


**Warning:** pause is not safe, it is better to use sigsuspend

**Warning2:** sigsuspend NIE GWARANTUJE WYKONANIA MAKSYMALNIE JEDNEJ OBSŁUGI SYGNAŁU! To częsty błąd w rozumowaniu! Zaraz po wywołaniu obsługi SIGUSR2 jeszcze w obrębie jednego wykonania sigsuspend następuje obsługa SIGUSR1, zmienna globalna jest nadpisywana i proces rodzic nie ma szansy zliczyć SIGUSR2!!!

## getppid
**Manpage:** man 3p getppid
```c
#include <unistd.h>

pid_t getppid(void);
```
**Description:** The getppid() function shall return the parent process ID of the calling process.

**Return value:** The getppid() function shall always be successful and no return value is reserved to indicate an error.

## sigprocmask
**Manpage:** man 3p pthread_sigmask
```c
#include <signal.h>

int sigprocmask(int how, const sigset_t *restrict set,
    sigset_t *restrict oset);
```
**Description:** In a single-threaded process, the sigprocmask() function shall examine or change (or both) the signal mask of the calling thread (process). The signal mask is a set of signals that are currently blocked and will not be delivered to the process.

**Input value:**
- `how`: an integer value that specifies how the signal mask should be changed. This can be one of the following values:
    - *SIG_BLOCK*: add the signals in set to the current signal mask (i.e., block the signals in set)
    - *SIG_UNBLOCK*: remove the signals in set from the current signal mask (i.e., unblock the signals in set)
    - *SIG_SETMASK*: replace the current signal mask with the signals in set
- `set`: a pointer to a sigset_t data structure that contains the set of signals to be added to, removed from, or replaced in the current signal mask, depending on the value of how
- `oldset`: a pointer to a sigset_t data structure that will be filled with the current signal mask. This argument is optional and can be NULL if the caller is not interested in the current signal mask.

**Return value:** Upon successful completion, sigprocmask() shall return 0; otherwise, -1 shall be returned, errno shall be set to indicate the error, and the signal mask of the process shall be unchanged.

## sigaddset
**Manpage:** man 3p sigaddset

```c
#include <signal.h>

int sigaddset(sigset_t *set, int signo);
```
**Description:** The sigaddset() function adds the individual signal specified by the `signo` to the signal set pointed to by `set`.

**Return value:** Upon successful completion, sigaddset() shall return 0; otherwise, it shall return -1 and set errno to indicate the error.

## sigemptyset
**Manpage:** man 3p sigemptyset
```c
#include <signal.h>

int sigemptyset(sigset_t *set);
```
**Description:** The  sigemptyset()  function  initializes  the  signal  set  pointed  to  by  `set`.

**Return value:** Upon successful completion, sigaddset() shall return 0; otherwise, it shall return -1 and set errno to indicate the error.

## open
**Manpage:** man 3p open, man 3p mknod
```c
#include <sys/stat.h>
#include <fcntl.h>

int open(const char *path, int oflag, mode_t mode);
int openat(int fd, const char *path, int oflag, mode_t mode);
```

**Description:** The  open()  function  shall  establish the connection between a file and a file descriptor.

**Input values:**
- `fd` - directory file descriptor, if provided the following path will be relative to this directory 
- `path` - path to the file
- `oflag` - type of access, to combine flags, use OR operation

| Flag             | Description                                                                                            |
|------------------|--------------------------------------------------------------------------------------------------------|
| `O_RDONLY`       | Open the file for reading only.                                                                         |
| `O_WRONLY`       | Open the file for writing only.                                                                         |
| `O_RDWR`         | Open the file for reading and writing.                                                                  |
| `O_APPEND`       | Append new data to the end of the file.                                                                  |
| `O_CREAT`        | Create the file if it does not already exist.                                                           |
| `O_EXCL`         | Ensure that the file is created exclusively (i.e., the operation will fail if the file already exists). |
| `O_TRUNC`        | Truncate the file to zero length if it already exists and is a regular file.                             |
| `O_SYNC`         | Write data to the file in synchronous mode.                                                             |
| `O_NONBLOCK`     | Open the file in non-blocking mode.                                                                     |
| `O_DSYNC`        | Write data to the file in synchronized mode.                                                            |

- `mode` - when using *O_CREAT*, flag permission flags should be also provided, in order to combine them use OR operation

| Constant          | Description                                                                                    |
|-------------------|------------------------------------------------------------------------------------------------|
| `S_IRUSR`         | Read permission for the owner of the file.                                                     |
| `S_IWUSR`         | Write permission for the owner of the file.                                                    |
| `S_IXUSR`         | Execute permission for the owner of the file.                                                  |
| `S_IRGRP`         | Read permission for the group associated with the file.                                        |
| `S_IWGRP`         | Write permission for the group associated with the file.                                       |
| `S_IXGRP`         | Execute permission for the group associated with the file.                                     |
| `S_IROTH`         | Read permission for other users (i.e., users who are not the owner or part of the group).      |
| `S_IWOTH`         | Write permission for other users (i.e., users who are not the owner or part of the group).     |
| `S_IXOTH`         | Execute permission for other users (i.e., users who are not the owner or part of the group).   |
| `S_IRWXU`         | Read, write, and execute permission for the owner of the file.                                 |
| `S_IRWXG`         | Read, write, and execute permission for the group associated with the file.                    |
| `S_IRWXO`         | Read, write, and execute permission for other users.                                           |

**Return value:**

Upon successful completion, these functions shall open the file and return a non-negative integer representing the file descriptor.  

Otherwise, these functions shall return -1 and set errno to indicate the error. If -1 is returned, no files shall be created or modified.

**Examples:**

```c
// Open a file for writing, creating it if it doesn't exist
int fd = open("myfile.txt", O_WRONLY | O_CREAT);
```

```c
// Open the directory "/tmp"
int dir_fd = open("/tmp", O_RDONLY);

// Open the file "myfile.txt" in the directory "/tmp" (second argument is a path
// relative to the path provided in the first open)
int fd = openat(dir_fd, "./myfile.txt", O_RDONLY);

// Does the same thing
int fd = openat(dir_fd, "myfile.txt", O_RDONLY);
```

```c
#include <fcntl.h>
// ...
int fd;
mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
char *pathname = "/tmp/file";
// ...
fd = open(pathname, O_WRONLY | O_CREAT | O_TRUNC, mode);
// ...
```

## close
**Manpage:** man 3p close

```c
#include <unistd.h>

int close(int fildes);
```

**Description:** The close() function shall deallocate the file descriptor indicated by fildes. To deallocate means to make the file descriptor available for return by subsequent calls to open() or other functions that allocate file descriptors.

**Return value:** Upon successful completion, 0 shall be returned; otherwise, -1 shall be returned and errno set to indicate  the  error.

## read
**Manpage:** man 3p read

```c
#include <unistd.h>

ssize_t pread(int fildes, void *buf, size_t nbyte, off_t offset);
ssize_t read(int fildes, void *buf, size_t nbyte);
```
**Description:**

The  read()  function  shall  attempt  to  read  *nbyte* bytes from the file associated with the open file descriptor, *fildes*, into the buffer pointed to by *buf*.

The pread() function allows providing *offset* from the start of the file

**Return value:**

Upon successful completion, these functions shall return a non-negative integer indicating the number of bytes actually read. 

Otherwise, the functions shall return -1 and set errno to indicate the error.

**Example:**

```c
#include <unistd.h>
#include <fcntl.h>

#define BUFFER_SIZE 1024

int main() {
    int fd;
    char buffer[BUFFER_SIZE];

    // Open the file
    fd = open("file.txt", O_RDONLY);
    if (fd == -1) {
        // Handle error
    }

    // Read from the file at offset 10
    ssize_t bytes_read = pread(fd, buffer, BUFFER_SIZE, 10);
    if (bytes_read == -1) {
        // Handle error
    }

    // Do something with the data that was read
    // ...

    // Close the file
    if (close(fd) == -1) {
        // Handle error
    }

    return 0;
}
```

## write
**Manpage:** man 3p write

```c
#include <unistd.h>

ssize_t pwrite(int fildes, const void *buf, size_t nbyte,
    off_t offset);
ssize_t write(int fildes, const void *buf, size_t nbyte);
```

**Description:** The  write()  function  shall  attempt to write nbyte bytes from the buffer pointed to by buf to the file associated with the open file descriptor, fildes.

**Return value:**

Upon successful completion, these functions shall return the number of bytes actually written to the file associated with *fildes*. This number shall never be greater than *nbyte*.  

Otherwise, -1 shall be returned and errno set to indicate the error.

**Example:**

```c
#include <unistd.h>
#include <fcntl.h>

#define BUFFER_SIZE 1024

int main() {
    int fd;
    char buffer[BUFFER_SIZE] = "Hello, world!";

    // Open the file
    fd = open("file.txt", O_WRONLY | O_CREAT);
    if (fd == -1) {
        // Handle error
    }

    // Write to the file at offset 10
    ssize_t bytes_written = pwrite(fd, buffer, BUFFER_SIZE, 10);
    if (bytes_written == -1) {
        // Handle error
    }

    // Close the file
    if (close(fd) == -1) {
        // Handle error
    }

    return 0;
}
```

## urandom
**Manpage:** man 4 urandom

```c
#include <linux/random.h>

int ioctl(fd, RNDrequest, param);
```

The  character  special  files /dev/random and /dev/urandom provide an interface to the kernel's random number generator.

- random - high quality, but slow
- urandom - low quality, but fast 

**Example:**
```c
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>

int main() {
    int fd;
    int random_number;

    // Open the /dev/random device file
    fd = open("/dev/random", O_RDONLY);
    if (fd == -1) {
        // Handle error
    }

    // Read a random number from the file
    if (ioctl(fd, RNDGETENTCNT, &random_number) == -1) {
        // Handle error
    }

    // Do something with the random number
    // ...

    // Close the file
    if (close(fd) == -1) {
        // Handle error
    }

    return 0;
}
```

## TEMP_FAILURE_RETRY
**GNU manual:** https://www.gnu.org/software/libc/manual/html_node/Interrupted-Primitives.html

**Description** A signal can arrive and be handled while an I/O primitive such as open or read is waiting for an I/O device. If the signal handler returns, the system faces the question: what should happen next?

The GNU C Library provides a convenient way to retry a call after a temporary failure, with the macro TEMP_FAILURE_RETRY: 

```c
#define _GNU_SOURCE
#include <unistd.h>
TEMP_FAILURE_RETRY (expression)
```

This macro evaluates expression once, and examines its value as type long int. If the value equals -1, that indicates a failure and errno should be set to show what kind of failure. If it fails and reports error code EINTR, TEMP_FAILURE_RETRY evaluates it again, and over and over until the result is not a temporary failure. 

The value returned by TEMP_FAILURE_RETRY is whatever value expression produced. 

**Examples:**
```c
ssize_t bulk_read(int fd, char *buf, size_t count)
{
	ssize_t c;
	ssize_t len = 0;
	do {
		c = TEMP_FAILURE_RETRY(read(fd, buf, count));
		if (c < 0)
			return c;
		if (c == 0)
			return len; // EOF
		buf += c;
		len += c;
		count -= c;
	} while (count > 0);
	return len;
}

ssize_t bulk_write(int fd, char *buf, size_t count)
{
	ssize_t c;
	ssize_t len = 0;
	do {
		c = TEMP_FAILURE_RETRY(write(fd, buf, count));
		if (c < 0)
			return c;
		buf += c;
		len += c;
		count -= c;
	} while (count > 0);
	return len;
}
```