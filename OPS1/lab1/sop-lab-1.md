| &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp;&nbsp; &nbsp; &nbsp; &nbsp; &nbsp;| &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp;&nbsp; &nbsp; &nbsp; &nbsp; &nbsp;&nbsp; &nbsp; &nbsp;  &nbsp;&nbsp; &nbsp;  All | Links &nbsp; &nbsp; &nbsp;  &nbsp; &nbsp;&nbsp; &nbsp; &nbsp; &nbsp; &nbsp;&nbsp; &nbsp; &nbsp; &nbsp; &nbsp;&nbsp; &nbsp; &nbsp; &nbsp; &nbsp;| &nbsp; &nbsp; &nbsp; &nbsp; &nbsp;&nbsp; &nbsp; &nbsp; &nbsp; &nbsp;&nbsp; &nbsp; &nbsp; &nbsp; &nbsp;&nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp;   |
|---------|-|-|-----|
| [stdin](#stdin-3p) | [printf, fprintf](#printf-fprintf-3p) | [make](#make)      | [fscanf](#fscanf-3p) |
| [perror](#perror-3p) | [fgets](#fgets-3p) | [xargs](#xargs-1) | [exit](#exit-3p) |
| [atoi](#atoi-3p) |  [strtol](#strtol-3p) | [getopt](#getopt-3p) | [environ](#environ-3p-7) |
| [getenv](#getenv-3p) | [putenv](#putenv-3p) | [setenv](#setenv-3p) | [system](#system-3) | 
| [opendir](#opendir-3p) | [closedir](#closedir-3p) | [readdir](#readdir-3p) | [stat, lstat](#stat-lstat-3p)
| [errno](#errno-3p) | [getcwd](#getcwd-3p) | [ftw](#ftw-3p) | [nftw](#nftw-3p)
| [fopen](#fopen-3p) | [fclose](#fclose-3p) | [fseek](#fseek-3p) | [rand](#rand-3p) 
| [unlink](#unlink-3p) | [umask](#umask-3p) |

```c
    errno = 0;
    char *endptr = NULL;
    depth = strtoul(optarg, &endptr, 8);
    if (errno != 0 || endptr == optarg || *endptr != '\0')
        ERR("The strtol() function error");
```
```c
#define ERR(source) (perror(source), \
fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), \
exit(EXIT_FAILURE))
```
# stdin (3p) 
> standard I/O streams
 ```c
#include <stdio.h>

extern FILE *stderr, *stdin, *stdout;
```

The following symbolic values in `<unistd.h>` define the file descriptors when the application is started:
```c
STDIN_FILENO  Standard input value, stdin.  Its value is 0.
STDOUT_FILENO Standard output value, stdout.  Its value is 1.
STDERR_FILENO Standard error value, stderr.  Its value is 2.
```

# printf, fprintf (3p)
> print formatted output

`printf()` function shall place output on the standard output stream `stdout`

`fprintf()` function shall place output on the named output `stream`
```c
#include <stdio.h>

int fprintf(FILE *restrict stream, const char *restrict format, ...);
int printf(const char *restrict format, ...);
```
### Description:
The format is composed of zero or more directives: 
- __ordinary characters__, which are simply copied to the output stream
- __conversion specifications__, each of which shall result in the fetching of zero or more arguments

The format can contain either numbered argument conversion
       specifications (that is, `%n$` and `*m$`), or unnumbered argument
       conversion specifications (that is, `%` and `*`), but not both.

Each conversion specification is introduced by the `%` character
or by the character sequence `%n$`, after which the following
appear in sequence:

-  flags (in any order), which modify the meaning of the conversion specification.

- an optional minimum field width.  If the converted value has fewer bytes than the field width, it shall be padded with _space_ characters by default on the left; it shall be padded on the right if thre is an left-adjustment flag (`-`). The field width takes the form of an _asterisk_ ('*') or a decimal integer.

- an optional precision that gives
    - the minimum number of digits (significant digits for the `g` and `G` conversion specifiers)
    - the maximum number of bytes to be printed from a string in the `s` and `S` conversion specifiers

    > If a precision appears with any other conversion specifier, the behavior is __undefined__.

- an optional length modifier that specifies the size of the argument

- a conversion specifier

### Invalid input
- insufficient arguments for the format: results are undefined 
- format exhausted while arguments remain: the excess arguments shall
be evaluated but are otherwise ignored

### Return value
 > number of bytes transmitted


# make
> GNU make utility to maintain groups of programs
```bash
make [OPTION]... [TARGET]...
```
### Description
`make` executes commands in the makefile to update one or more target names, where name is typically a program.  If no `-f` option is present, make will look for the makefiles _GNUmakefile, makefile,_ and _Makefile,_ in that order.

`make` updates a target if it depends on prerequisite files that have been modified since the target was last modified, or if the target does not exist.


### Exit status 
> `0` if all makefiles were successfully parsed and no targets that were built failed

> `1` if the `-q` flag was used and make determines that a target needs to be rebuilt

> `2` if any errors were encountered

### Makefile (with flags required for labs)
```bash
CC=gcc
CFLAGS=-Wall -fsanitize=address,undefined
LDFLAGS=-fsanitize=address,undefined
```

### Makefile for multiple files

```bash
# c compiler you want to use
CC=gcc
# compiling flags
C_FLAGS=-Wall -fsanitize=address,undefined -pedantic -Os  
# linking flags
L_FLAGS=-fsanitize=address,undefined
# executable file name
TARGET=my_program
FILES=main.o hello.o

all: ${TARGET}

${TARGET}: ${FILES}
	${CC} -o ${TARGET} ${FILES} ${L_FLAGS}

main.o: main.c hello.h
	${CC} -o main.o -c main.c ${C_FLAGS}

hello.o: hello.c hello.h
	${CC} -o hello.o -c hello.c ${C_FLAGS}

# .PHONY prevents situations when there is a file called "clean" and instead of running
# this command, information about the "clean" file will be shown
.PHONY: clean run

# '-' sign prevent from showing errors 
clean:
	-rm -f ${FILES} ${TARGET}
```

# fscanf (3p)
> convert formatted input
```c
#include <stdio.h>

int fscanf(FILE *restrict stream, const char *restrict format, ...);
int scanf(const char *restrict format, ...);
int sscanf(const char *restrict s, const char *restrict format, ...);
```
### Description
`fscanf()`  shall read from the named input `stream`

`scanf()`  shall read from the standard input stream `stdin` 

Each function reads bytes, interprets them according to a format, and stores  the  results in  its arguments. 

Each expects, as arguments, a __control string__ `format` and a __set of pointer arguments__ indicating where the converted input should be stored. 

### Invalid input
- insufficient arguments for the format: results are undefined 
- format exhausted while arguments remain: the excess arguments shall
be evaluated but are otherwise ignored

### Examples
(1) The call:
```c
    int i, n; float x; char name[50];
    n = scanf("%d%f%s", &i, &x, name);
```

with the input line:

```    
    25 54.32E-1 Hamster
```

assigns to n the value 3, to i the value 25, to x the value
5.432, and name contains the string "Hamster".

(2) The call:
```c
    int i; float x; char name[50];
    (void) scanf("%2d%f%*d %[0123456789]", &i, &x, name);
```
with input:
```
    56789 0123 56a72
```
assigns `56` to `i`, `789.0` to x, skips `0123`, and places the string
`"56\0"` in name.  The next call to `getchar()` shall return the
character _a_.

# perror (3p)
> write error messages to standard error
```c
#include <stdio.h>

void perror(const char *s);
```
### Description
`perror()` shall map the error number accessed through
the symbol `errno` to an error message

Message shall be written to the standard error stream as follows:

1. the string pointed to by `s` 

2.  an error message string 

The contents of the error message strings shall be the same as
those returned by `strerror()` with argument `errno`

 # opendir (3p)
 > open directory associated with file descriptor
 ```c
#include <dirent.h>

 DIR *opendir(const char *dirname);
```
### Description
`opendir()` shall open a _directory stream_ corresponding to the directory named by the `dirname` argument. The directory stream is positioned at the first entry. Applications shall only be able to open up to `{OPEN_MAX}` files and directories.

### Return value
 > pointer to an object of type `DIR`

 ### Example 
 Opening a Directory Stream:
 ```c
#include <dirent.h>
    ...
        DIR *dir;
        struct dirent *dp;
    ...
        if ((dir = opendir (".")) == NULL) {
            perror ("Cannot open .");
            exit (1);
        }

        while ((dp = readdir (dir)) != NULL) {
            ...
        }
 ```
# closedir (3p)
> close a directory stream
```c
#include <dirent.h>

int closedir(DIR *dirp);
```
### Description
`closedir()` shall close the directory stream referred to by the argument `dirp`
### Return value
 > `0` upon successful completion

 > `-1` otherwise (`errno` is set)


# readdir (3p)
> read a directory
```c
#include <dirent.h>

struct dirent *readdir(DIR *dirp);
```
### Description
`readdir()` shall return a pointer to a structure representing the directory entry at the current position in the directory stream specified by the argument `dirp` (type `DIR` represents a _directory stream_), and position the directory stream at the next entry. 

`struct dirent` contains only 2 fields: `d_name` and `d_ino` (inode number).

### Return value
 > pointer to an object of type `struct dirent`

 > `null` if the end of the directory is encountered

 > `null` (and  `errno` set) there was an error


# stat, lstat (3p)
> get file status
```c
#include <fcntl.h>
#include <sys/stat.h>

int lstat(const char *restrict path, struct stat *restrict buf);
int stat(const char *restrict path, struct stat *restrict buf); 
```
### Description
`stat()` shall obtain information about the file and write it to the area pointed to by `buf` (pointer to a `stat` structure). The `path` argument points to a _pathname_ naming a file. Read, write, or execute permission is not required.

`lstat()` shall be equivalent to `stat()`, except when path refers to a symbolic link. In that case `lstat()` shall return information about the link, while `stat()` shall return information about the file the link references.


`lstat()` - feature test macro requirements:
```bash
_DEFAULT_SOURCE || _XOPEN_SOURCE >= 500 || _POSIX_C_SOURCE >= 200112L
```

### Example
 Obtaining file status information for a file named `/home/cnd/mod1`
 ```c
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

struct stat buffer;
int         status;
...
status = stat("/home/cnd/mod1", &buffer);
```

### Return value
 > `0` upon successful completion

 > `-1` otherwise (`errno` is set)

# errno (3p)
> error return value
```c
#include <errno.h>
```
### Description
_lvalue_ `errno` is used by many functions to return error values

functions provide an error number in `errno`, which has type `int` and is defined in `<errno.h>`

### Example
 Obtaining file status information for a file named `/home/cnd/mod1`
 ```c
    file = fopen(fname, "r");
    if (file == NULL) {
        fprintf(stderr, "Error while trying to open '%s': %s\n", fname, strerror(errno));
    } else {
        fprintf (stderr, "'%s' opened successfully\n", fname);
        fclose (file);
    }
```

# getcwd (3p)
> get the pathname of the current working directory
```c
#include <unistd.h>

char *getcwd(char *buf, size_t size);
```
### Description
`getcwd()` shall place an _absolute_ (no components that are _dot_ or _dot-dot_, or are _symbolic links_) pathname of the current working directory in the array pointed to by `buf` (of size `size`), and return `buf`.

### Return value
 > `buf` upon successful completion

 > null pointer otherwise (`errno` is set)

# chdir (3p)
> change working directory
```c
#include <unistd.h>

int chdir(const char *path);
```
### Description
`chdir()` shall cause the directory named by the pathname pointed to by the `path` argument to become the _current working directory_ (the starting point for path searches for pathnames not beginning with '/').
### Example

### Return value
 > `0` upon successful completion

 > `-1` otherwise (`errno` is set)


 # ftw (3p)
> recursively traverse (walk) a file tree
```c
#include <ftw.h>

int ftw(const char *path, int (*fn)(const char *, const struct stat *ptr, int flag), int ndirs);
```
### Description
`ftw()` shall recursively descend the directory hierarchy rooted in `path`.  For each object, `ftw()` shall call the function pointed to by `fn`, passing it a pointer to a  string containing the name of the object, a pointer to a `stat` structure, filled in as if `stat()` or `lstat()` had been called to retrieve the information.

Possible values of the integer, defined in the  `<ftw.h>` header, are:
- `FTW_D`     for a directory
- `FTW_DNR`   for a directory that cannot be read
- `FTW_F`    for a non-directory file
- `FTW_SL`    for a symbolic link 
- `FTW_NS`    for an object other than a symbolic link on which `stat()` could not successfully be execute

`ftw()` shall visit a directory before visiting any of its descendants.
The tree traversal shall continue until either the tree is exhausted, an invocation of `fn` returns a non-zero value, or some error is detected.

### Return value
 > `0` if the tree is exhausted

> if the function pointed to by `fn` returns a non-zero value, function shall return that value

 > `-1` on error (`errno` is set)

 # nftw (3p)
> walk a file tree
```c
#include <ftw.h>

int nftw(const char *path, int (*fn)(const char *, const struct stat *, int, struct FTW *), int fd_limit, int flags); 
```
### Description
`nftw()` shall recursively descend the directory hierarchy rooted in `path`. It has a similar effect to `ftw()` except that it takes an additional argument flags, which is a bitwise-inclusive OR of zero or more of the following flags:
- `FTW_CHDIR `  -  `nftw()` shall change the current working
directory to each directory as it reports files in
that directory.

- `FTW_DEPTH `  -  `nftw()` shall report all files in a directory
before reporting the directory itself. If clear,
`nftw()` shall report any directory before reporting
the files in that directory.

- `FTW_MOUNT`   -  `nftw()` shall only report files in the same
file system as path.  If clear, `nftw()` shall report
all files encountered during the walk.

- `FTW_PHYS`    -  `nftw()` shall perform a physical walk and
shall not follow symbolic links.

### Return value
 > `0` if the tree is exhausted

> if the function pointed to by `fn` returns a non-zero value, function shall return that value

 > `-1` on error (`errno` is set)

 # fgets (3p)
> get a string from a stream
```c
#include <stdio.h>

char *fgets(char *restrict s, int n, FILE *restrict stream); 
```
### Description
`fgets()`  shall read bytes from stream into the array
pointed to by `s` until `n-1` bytes are read, or a `<newline>` is read
and transferred to `s`, or an `end-of-file` condition is encountered.
A `null byte` shall be written immediately after the last byte read
into the array.  If the `end-of-file` condition is encountered
before any bytes are read, the contents of the array pointed to
by `s` shall not be changed.
### Example

### Return value
 > `s` upon successful completion

> null pointer if stream is at `eof`

 > null pointer otherwise (`errno` is set)

 # xargs (1)
> build and execute command lines from standard input
```
xargs [options] [command [initial-arguments]]
```
### Description
`xargs` reads
items from the standard input, delimited by blanks (which can be
protected with double or single quotes or a backslash) or
newlines, and executes the command (default is `echo`) one or more
times with any initial-arguments followed by items read from
standard input

### Examples

```bash
find /tmp -name core -type f -print | xargs /bin/rm -f
```
Find files named `core` in or below the directory `/tmp` and delete
them.  Note that this will work incorrectly if there are any
filenames containing newlines or spaces.

```bash
cat dane.txt | xargs ./prog4
```
Create arguments for `prog4` from file `dane.txt` contents (every word is a different argument).
```bash
cat dane.txt |tr "\n" "\0"| xargs -0 ./prog4
```
Create arguments for `prog4` from file `dane.txt` contents (every line is a different argument).

### Exit status
>`0`      if it succeeds

>`123`    if any invocation of the command exited with status
        1-125

>`124`    if the command exited with status 255

>`125`    if the command is killed by a signal

>`126`    if the command cannot be run

>`127`    if the command is not found

>`1`     if some other error occurred.

Exit codes greater than 128 are used by the shell to indicate
that a program died due to a fatal signal.

 # exit (3p)
> terminate a process
```c
#include <stdlib.h>

void exit(int status);
void _Exit(int status);
```
```c
#include <unistd.h>

void _exit(int status);
```
### Description
`exit()` shall first call all functions registered by
`atexit()`, in the reverse order of their registration. Function shall then flush all open streams with unwritten buffered data and close all open streams, then the process shall be terminated.

`_Exit()` and `_exit()` functions shall __not__ call functions
registered with `atexit()`, open streams shall not be flushed.


 # atoi (3p)
> convert a string to an integer
```c
#include <stdlib.h>

int atoi(const char *nptr);
long atol(const char *nptr);
long long atoll(const char *nptr);
```
### Description
`atoi()` converts the initial portion of the string
pointed to by nptr to int.  The behavior is the same as

```c
strtol(nptr, NULL, 10);
```

except that `atoi()` does not detect errors.

The `atol()` and `atoll()` functions behave the same as `atoi()`,
except that they return `long` or `long long`.

### Return value
 > converted value upon successful completion

 > `0` otherwise 

 # strtol (3p)
> convert a string to a long integer
```c
#include <stdlib.h>

long strtol(const char *restrict nptr, char **restrict endptr, int base);
long long strtoll(const char *restrict nptr, char **restrict endptr, int base);
```
### Description
These functions shall convert the initial portion of the string
       pointed to by `nptr` to a type `long` and `long long` representation,
       respectively.
### Example

### Return value
 > converted value upon successful completion

 > `0` otherwise (`errno` is set)

 # getopt (3p)
> command option parsing
```c
#include <unistd.h>

int getopt(int argc, char * const argv[], const char *optstring);
extern char *optarg;
extern int opterr, optind, optopt;
```
### Description
Parameters `argc` and `argv` are the argument count and argument
array as passed to `main()`.  

The argument `optstring`
is a string of recognized option characters; if a character is
followed by a `<colon>`, the option takes an argument.

The variable optind is the index of the next element of the
`argv[]` vector to be processed. It shall be initialized to `1` by
the system, and `getopt()` shall update it when it finishes with
each element of `argv[]`.


### Example
```c
    while ((c = getopt(argc, argv, ":abf:o:")) != -1) {
        switch(c) {
        case 'a':
            if (bflg)
                errflg++;
            else
                aflg++;
            break;
        case 'b':
            if (aflg)
                errflg++;
            else
                bflg++;
            break;
        case 'f':
            ifile = optarg;
            break;
        case 'o':
            ofile = optarg;
            break;
        case ':':       /* -f or -o without operand */
            fprintf(stderr,
                "Option -%c requires an operand\n", optopt);
            errflg++;
            break;
        case '?':
            fprintf(stderr,
                "Unrecognized option: '-%c'\n", optopt);
            errflg++;
```
This code accepts any of the following as equivalent:
```bash
cmd -ao arg path path
cmd -a -o arg path path
cmd -o arg -a path path
cmd -a -o arg -- path path
cmd -a -oarg path path
cmd -aoarg path path
```

### Return value
 > the next option character specified on the command line upon successful completion

> `: <colon>` when `getopt()` detects a missing argumentand the first character of optstring was a `: <colon>`

> `? <question-mark>`  shall be returned if `getopt()` encounters
an option character not in optstring or detects a missing
argument and the first character of optstring was __not__ a `<colon>`.

 > `-1` when all command line options are parsed


 # environ (3p, 7)
> array of character pointers to the environment strings (3p), user environment (7)
```c
extern char **environ;
```
### Description
`environ` points to an array of pointers to strings
called the "environment".  The last pointer in this array has the
value `NULL`.  This array of strings is made available to the
process by the `execve()` call when a new program is started.


When a child process is created via `fork()`, it inherits a __copy__
of its parent's environment.

By convention, the strings in environ have the form `"name=value"`.
The name is case-sensitive and may not contain the character "=".

 # getenv (3p)
> get value of an environment variable
```c
#include <stdlib.h>

char *getenv(const char *name);
```
### Description
`getenv()`  shall search the environment of the calling
       process for the environment variable` name` if it
       exists and return a pointer to the value of the environment
       variable. The application shall ensure
       that it does not modify the string pointed to by the `getenv()`
       function.

### Example
The following example gets the value of the `HOME` environment
variable.
```c
    #include <stdlib.h>
    ...
    const char *name = "HOME";
    char *value;

    value = getenv(name);
```

### Return value
 > pointer to a string containing the `value` for `name` upon successful completion

 > null pointer if `name` can't be found in the environment of the calling process


# putenv (3p)
> change or add a value to an environment
```c
#include <stdlib.h>

int putenv(char *string);
```
### Description
`putenv()` shall use the `string` argument to set
environment variable values. The string argument should point to
a string of the form `name=value`.  The `putenv()` function shall
make the value of the environment variable name equal to value by
altering an existing variable or creating a new one.
### Example
The following example changes the value of the `HOME` environment
variable to the value` /usr/home`.
```c
#include <stdlib.h>
...
static char *var = "HOME=/usr/home";
int ret;

ret = putenv(var);
```
### Return value
 > `0` upon successful completion

 > non-zero otherwise (`errno` is set)

 # setenv (3p)
> add or change environment variable
```c
#include <stdlib.h>

int setenv(const char *envname, const char *envval, int overwrite);
```
### Description
`setenv()` shall update or add a variable in the
environment of the calling process. The `envname` argument points
to a string containing the name of an environment variable to be
added or altered. The environment variable shall be set to the
value to which `envval` points. The function shall fail if `envname`
points to a string which contains an '=' character.

If theenvironment variable named by `envname` already exists and the
value of overwrite is non-zero, the function shall return success
and the environment shall be updated, if it is
zero, the function shall return success and the environment shall
remain unchanged.

The `setenv()` function shall update the list of pointers to which
`environ` points.
### Example

### Return value
 > `0` upon successful completion

 > `-1` otherwise (`errno` is set)

 # system (3)
> execute a shell command
```c
#include <stdlib.h>

int system(const char *command);
```
### Description
`system()` library function uses `fork()` to create a child
process that executes the shell command specified in command
using `execl()` as follows:
```bash
execl("/bin/sh", "sh", "-c", command, (char *) NULL);
```
`system()` returns after the command has been completed.

### Return value
The return value of system() is one of the following:

> if command is NULL, then a `nonzero` value if a shell is
available, or `0` if no shell is available.

>  If a child process could not be created, or its status could
not be retrieved, the return value is `-1` and `errno` is set.

>  If a shell could not be executed in the child process, then
the return value is as though the child shell terminated by
calling `_exit()` with the status `127`.

>  If all system calls succeed, then the return value is the
termination status of the child shell used to execute command.
(The termination status of a shell is the termination status
of the last command it executes.)

 In the last two cases, the return value is a _wait status_.

 # fopen (3p)
> open a stream
```c
#include <stdio.h>

FILE *fopen(const char *restrict pathname, const char *restrict mode);
```
### Description
`fopen()` shall open the file whose pathname is the string pointed to by `pathname`, and associates a stream with it. The `mode` argument points to a string:
- `r`, `rb`       - open file for reading.

- `w`, `wb`        - truncate to zero length or create file for writing

- `a`, `ab`      - append; open or create file for writing at `eof`

- `r+`, `rb+`, `r+b`  - open file for update (reading and writing)

- `w+`, `wb+`, `w+b`  - truncate to zero length or create file for update

- `a+`, `ab+`, `a+b` - append; open or create file for update, writing at `eof`

The character _b_ shall have no effect, but is allowed for ISO C standard conformance.

Opening a file with read mode  shall fail if the file does not exist or cannot be read. When a file is opened with update mode ('+' as the second in the `mode` argument), both input and output may
be performed on the associated stream.
If `mode` starts with `w` or `a`, and the
file did not previously exist, the `fopen()` function shall create
a file as if it called the `creat()` function with a value
appropriate for the path argument interpreted from `pathname`.

### Return value
 > pointer to the object controlling the stream upon successful completion

 > nullpointer otherwise (`errno` is set)

 # fclose (3p)
> close a stream
```c
#include <stdio.h>

int fclose(FILE *stream);
```
### Description
`fclose()` shall cause the stream pointed to by `stream`
to be flushed and the associated file to be closed. Any unwritten
buffered data for the stream shall be written to the file; any
unread buffered data shall be discarded.

### Return value
 > `0` upon successful completion

 > `eof` otherwise (`errno` is set)


 # fseek (3p)
> reposition a file-position indicator in a stream
```c
#include <stdio.h>

int fseek(FILE *stream, long offset, int whence);
```
### Description
`fseek()` shall set the file-position indicator for
the stream pointed to by `stream`.  If a read or write error
occurs, the error indicator for the stream shall be set and
`fseek()` fails.

The new position, measured in bytes from the beginning of the
file, shall be obtained by adding `offset` to the position
specified by `whence`.  The specified point is the beginning of the
file for `SEEK_SET`, the current value of the file-position
indicator for `SEEK_CUR`, or end-of-file for `SEEK_END`.

The `fseek()` function shall allow the file-position indicator to
be set __beyond the end of existing data in the file__. If data is
later written at this point, subsequent reads of data in the gap
shall return bytes with the value `0` until data is actually
written into the gap.

### Return value
 > `0` upon successful completion

 > `-1` otherwise (`errno` is set)

 # rand (3p)
> pseudo-random number generator
```c
#include <stdlib.h>

int rand(void);
int rand_r(unsigned *seed);
void srand(unsigned seed);
```
### Description
`rand()` and `rand_r()` shall compute a sequence of pseudo-random
integers in the range [0,{RAND_MAX}].
`srand()` function uses the argument as a seed for a new
sequence of pseudo-random numbers to be returned by subsequent
calls to `rand()`. 

If `srand()` is called with the same seed
value, the sequence of pseudo-random numbers shall be repeated. If `rand_r()` is called with the same object pointed to by seed the same sequence shall
be generated.

If `rand()` is called before any calls to `srand()` are made, the
same sequence shall be generated as when `srand()` is first called
with a seed value of 1.

### Return value
 > `rand()`: next pseudo-random number in
       the sequence

 > `rand_r()`: pseudo-random integer

 # unlink (3p)
> remove a directory entry
```c
#include <unistd.h>

int unlink(const char *path);
```
```c
#include <fcntl.h>

int unlinkat(int fd, const char *path, int flag);
```
### Description
`unlink()` shall remove a link to a file. If path
names a symbolic link, `unlink()` shall remove the symbolic link
named by path and shall not affect any file or directory named by
the contents of the symbolic link. Otherwise,` unlink()` shall
remove the link named by the pathname pointed to by `path` and
shall decrement the link count of the file referenced by the
link.

When the file's link count becomes 0 and no process has the file
open, the space occupied by the file shall be freed and the file
shall no longer be accessible.

`unlinkat()` shall be equivalent to the `unlink()` or
`rmdir()` except in the case where path specifies a
relative path. In this case the directory entry to be removed is
determined relative to the directory associated with the file
descriptor `fd` instead of the current working directory.
### Example

The following example shows how to remove a link to a file named
`/home/cnd/mod1` by removing the entry named `/modules/pass1`.
```c
#include <unistd.h>

char *path = "/modules/pass1";
int   status;
...
status = unlink(path);
```
### Return value
 > `0` upon successful completion

 > `-1` otherwise (`errno` is set)


 # umask (3p)
> set and get the file mode creation mask
```c
#include <sys/stat.h>

mode_t umask(mode_t cmask);
```
### Description
`umask()` shall set the file mode creation mask of the
process to `cmask` and return the previous value of the mask. Only
the file permission bits of `cmask` are used.
### Example

### Return value
 > value which has file permission bits of previous mask (state of any other bits in that value is unspecified)