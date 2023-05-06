```toc
```
## aio (7)

> Asynchronous Input and Output

The `aio` library provides a set of functions that allow for the asynchronous execution of input and output operations. These functions allow a program to initiate an I/O operation and then proceed to execute other operations while the I/O is in progress. Once the I/O operation completes, the program can be notified and retrieve the results.

### Key functions

-   `aio_read()`: initiates an asynchronous read operation
-   `aio_write()`: initiates an asynchronous write operation
-   `aio_fsync()`: initiates an asynchronous fsync operation
-   `aio_error()`: retrieves the error status of an asynchronous I/O operation
-   `aio_return()`: retrieves the return status of an asynchronous I/O operation
-   `aio_suspend()`: waits for one or more asynchronous I/O operations to complete
-   `aio_cancel()`: cancels one or more asynchronous I/O operations

### Data structure

`struct aiocb` - describes an asynchronous I/O operation

```c
struct aiocb {
   int             aio_fildes;     /* File descriptor */
   off_t           aio_offset;     /* File offset */
   volatile void  *aio_buf;        /* Location of buffer */
   size_t          aio_nbytes;     /* Length of transfer */
   int             aio_reqprio;    /* Request priority */
   struct sigevent aio_sigevent;   /* Notification method */
   int             aio_lio_opcode; /* Operation to be performed; lio_listio() only */
};
```

## aio_read (3p)

> asynchronously read data from a file descriptor

```c
#include <aio.h>

int aio_read(struct aiocb *aiocbp);
```

`aio_read()` asynchronously reads data from a file descriptor into a buffer. The `aiocbp` argument is a pointer to an `aiocb` struct that describes the buffer, the file descriptor, and other information about the read operation.

### Return value

> `0` on success

> `-1` on error, with `errno` set to indicate the error

### Example

This example reads data from a file descriptor asynchronously and prints the read data to the console.

```c
#include <stdio.h>
#include <aio.h>

#define BUFSIZE 100

int main(int argc, char *argv[])
{
    int fd;
    struct aiocb aiocb;
    char buf[BUFSIZE];
    
    /* Open a file */
    fd = open("file.txt", O_RDONLY);
    if (fd == -1) {
        perror("open");
        return 1;
    }
    
    /* Set up the aiocb struct */
    bzero(&aiocb, sizeof(struct aiocb));
    aiocb.aio_fildes = fd;
    aiocb.aio_buf = buf;
    aiocb.aio_nbytes = BUFSIZE;
    aiocb.aio_offset = 0;
    
    /* Start the read operation */
    if (aio_read(&aiocb) == -1) {
        perror("aio_read");
        return 1;
    }
    
    /* Wait for the read operation to complete */
    while (aio_error(&aiocb) == EINPROGRESS);
    
    /* Print the read data */
    printf("Data read: %s\n", buf);
    
    close(fd);
    return 0;
}

```

## aio_write (3p)

> asynchronously write data to a file descriptor

```c
#include <aio.h>

int aio_write(struct aiocb *aiocbp);
```

`aio_write()` asynchronously writes data from a buffer to a file descriptor. The `aiocbp` argument is a pointer to an `aiocb` struct that describes the buffer, the file descriptor, and other information about the write operation.

### Return value

> `0` on success

> `-1` on error, with `errno` set to indicate the error

### Example

This example writes a string to a file descriptor asynchronously.

```c
#include <stdio.h>
#include <aio.h>

#define BUFSIZE 100

int main(int argc, char *argv[])
{
    int fd;
    struct aiocb aiocb;
    char buf[BUFSIZE] = "Hello, world!";
    
    /* Open a file */
    fd = open("file.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {
        perror("open");
        return 1;
    }
    
    /* Set up the aiocb struct */
    bzero(&aiocb, sizeof(struct aiocb));
    aiocb.aio_fildes = fd;
    aiocb.aio_buf = buf;
    aiocb.aio_nbytes = BUFSIZE;
    aiocb.aio_offset = 0;
    
    /* Start the write operation */
    if (aio_write(&aiocb) == -1) {
        perror("aio_write");
        return 1;
    }
    
    /* Wait for the write operation to complete */
    while (aio_error(&aiocb) == EINPROGRESS);
    
    close(fd);
    return 0;
}
```

It's worth noting that `aio_write()` may not write all the data requested. The amount of data written can be obtained by calling `aio_return()` on the `aiocb` struct.

## aio_fsync (3p)

> asynchronously synchronize a file descriptor's in-memory state with that on disk

```c
#include <aio.h>

int aio_fsync(int op, struct aiocb *aiocbp);
```

`aio_fsync()` asynchronously synchronizes a file descriptor's in-memory state with that on disk. The `op` argument is either `O_SYNC` or `O_DSYNC` and indicates whether to synchronize the file's data and metadata or just the data. The `aiocbp` argument is a pointer to an `aiocb` struct that describes the file descriptor and other information about the synchronization operation.

### Return value

> `0` on success

> `-1` on error, with `errno` set to indicate the error

### Example

This example synchronizes a file descriptor's in-memory state with that on disk asynchronously.

```c
#include <stdio.h>
#include <aio.h>
#include <fcntl.h>

int main(int argc, char *argv[])
{
    int fd;
    struct aiocb aiocb;
    char buf[100] = "Hello, world!";
    
    /* Open a file */
    fd = open("file.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {
        perror("open");
        return 1;
    }
    
    /* Write some data to the file */
    if (write(fd, buf, sizeof(buf)) == -1) {
        perror("write");
        return 1;
    }
    
    /* Set up the aiocb struct */
    bzero(&aiocb, sizeof(struct aiocb));
    aiocb.aio_fildes = fd;
    
    /* Start the synchronization operation */
    if (aio_fsync(O_SYNC, &aiocb) == -1) {
        perror("aio_fsync");
        return 1;
    }
    
    /* Wait for the synchronization operation to complete */
    while (aio_error(&aiocb) == EINPROGRESS);
    
    close(fd);
    return 0;
}
```

It's worth noting that `aio_fsync()` is not guaranteed to synchronize all previously written data. It only synchronizes data written prior to the aio_fsync call.

## aio_error (3p)

> retrieve error status of an asynchronous I/O operation

```c
#include <aio.h>

int aio_error(const struct aiocb *aiocbp);
```

`aio_error()` retrieves the error status of an asynchronous I/O operation. The `aiocbp` argument is a pointer to an `aiocb` struct that describes the operation.

### Return value

> `0` on success

> `ECANCELED` if the operation was cancelled

> `EINPROGRESS` if the operation is still in progress

> error status (positive number) if operation has completed unsuccessfully

> `-1` if an error occurred, with `errno` set to indicate the error

### Example

This example checks the status of an asynchronous read operation after it has been started.

```c
#include <stdio.h>
#include <aio.h>

#define BUFSIZE 100

int main(int argc, char *argv[])
{
    int fd;
    struct aiocb aiocb;
    char buf[BUFSIZE];
    
    /* Open a file */
    fd = open("file.txt", O_RDONLY);
    if (fd == -1) {
        perror("open");
        return 1;
    }
    
    /* Set up the aiocb struct */
    bzero(&aiocb, sizeof(struct aiocb));
    aiocb.aio_fildes = fd;
    aiocb.aio_buf = buf;
    aiocb.aio_nbytes = BUFSIZE;
    aiocb.aio_offset = 0;
    
    /* Start the read operation */
    if (aio_read(&aiocb) == -1) {
        perror("aio_read");
        return 1;
    }
    
    /* Check the status of the operation */
    int status = aio_error(&aiocb);
    if (status == 0) {
        printf("Read operation completed successfully.\n");
    } else if (status == EINPROGRESS) {
        printf("Read operation is still in progress.\n");
    } else {
        printf("Read operation failed with error: %d\n", status);
    }
    close(fd);
    return 0;
}
```

It is worth noting that the `aio_error()` function returns the error status of the operation that was previously given as a parameter in the `aiocb` struct.

## aio_return (3p)

> retrieve return status of an asynchronous I/O operation

```c
#include <aio.h>

ssize_t aio_return(struct aiocb *aiocbp);
```

`aio_return()` retrieves the return status of an asynchronous I/O operation. The `aiocbp` argument is a pointer to an `aiocb` struct that describes the operation.

### Return value

> The number of bytes transferred on success

> `-1` if an error occurred, with `errno` set to indicate the error

### Example

This example retrieves the return status of an asynchronous write operation after it has been started.

```c
#include <stdio.h>
#include <aio.h>
#include <fcntl.h>

#define BUFSIZE 100

int main(int argc, char *argv[])
{
    int fd;
    struct aiocb aiocb;
    char buf[BUFSIZE] = "Hello, world!";
    
    /* Open a file */
    fd = open("file.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {
        perror("open");
        return 1;
    }
    
    /* Set up the aiocb struct */
    bzero(&aiocb, sizeof(struct aiocb));
    aiocb.aio_fildes = fd;
    aiocb.aio_buf = buf;
    aiocb.aio_nbytes = BUFSIZE;
    aiocb.aio_offset = 0;
    
    /* Start the write operation */
    if (aio_write(&aiocb) == -1) {
        perror("aio_write");
        return 1;
    }
    
    /* Wait for the write operation to complete */
    while (aio_error(&aiocb) == EINPROGRESS);
    
    /* Retrieve the return status of the operation */
    ssize_t bytes_written = aio_return(&aiocb);
    if (bytes_written == -1) {
        printf("Write operation failed with error: %d\n", errno);
    } else {
        printf("Write operation completed successfully, %ld bytes written\n", bytes_written);
    }
    close(fd);
    return 0;
}
```

It is worth noting that the `aio_return()` function retrieves the return status of the operation that was previously given as a parameter in the `aiocb` struct. It is only valid after the operation has completed.

## aio_suspend (3p)

> wait for one or more asynchronous I/O operations to complete

```c
#include <aio.h>

int aio_suspend(const struct aiocb *const list[], int nent, const struct timespec *timeout);
```

`aio_suspend()` waits for one or more asynchronous I/O operations to complete. The `list` argument is an array of pointers to `aiocb` structs that describe the operations, and `nent` is the number of elements in the array. The `timeout` argument is a pointer to a `timespec` struct that specifies the maximum amount of time to wait for the operations to complete.

### Return value

> `0` on success

> `-1` on error, with `errno` set to indicate the error

### Example

This example suspends the execution of a program until an asynchronous read operation completes.

```c
#include <stdio.h>
#include <aio.h>

#define BUFSIZE 100

int main(int argc, char *argv[])
{
    int fd;
    struct aiocb aiocb;
    char buf[BUFSIZE];
    
    /* Open a file */
    fd = open("file.txt", O_RDONLY);
    if (fd == -1) {
        perror("open");
        return 1;
    }
    
    /* Set up the aiocb struct */
    bzero(&aiocb, sizeof(struct aiocb));
    aiocb.aio_fildes = fd;
    aiocb.aio_buf = buf;
    aiocb.aio_nbytes = BUFSIZE;
    aiocb.aio_offset = 0;
    
    /* Start the read operation */
    if (aio_read(&aiocb) == -1) {
        perror("aio_read");
        return 1;
    }
    
    /* Suspend execution until the operation completes */
    const struct aiocb *aiocb_list[1] = {&aiocb};
    if (aio_suspend(aiocb_list, 1, NULL) == -1) {
        perror("aio_suspend");
        return 1;
    }
    
    /* Print the read data */
    printf("Data read: %s\n", buf); 
    close(fd); 
    return 0; 
}
```

It is worth noting that `aio_suspend()` suspends the execution of the calling thread until one of the operations in the `list` array completes or the `timeout` expires. It can also be interrupted by a signal. Also, the `aiocb struct` that is passed as a parameter should have the operation (`aio_read`, `aio_write` or `aio_fsync`) already started.

## aio_cancel (3p)

> cancel one or more asynchronous I/O operations

`aio_cancel()` cancels one or more asynchronous I/O operations associated with a file descriptor. The `fildes` argument is the file descriptor, and `aiocbp` argument is a pointer to an `aiocb` struct that describes the operation to cancel. If `aiocbp` is `NULL`, then all operations associated with the file descriptor are canceled.

### Return value

> `AIO_CANCELED` if the operation was successfully canceled

> `AIO_NOTCANCELED` if the operation could not be canceled

> `AIO_ALLDONE` if the operation had already completed

> `-1` on error, with `errno` set to indicate the error

### Example

This example cancels an asynchronous read operation before it has completed.

```c
#include <stdio.h>
#include <aio.h>

#define BUFSIZE 100

int main(int argc, char *argv[])
{
    int fd;
    struct aiocb aiocb;
    char buf[BUFSIZE];
    
    /* Open a file */
    fd = open("file.txt", O_RDONLY);
    if (fd == -1) {
        perror("open"); 
        return 1; 
    }
    /* Set up the aiocb struct */
	bzero(&aiocb, sizeof(struct aiocb));
	aiocb.aio_fildes = fd;
	aiocb.aio_buf = buf;
	aiocb.aio_nbytes = BUFSIZE;
	aiocb.aio_offset = 0;
	
	/* Start the read operation */
	if (aio_read(&aiocb) == -1) {
	    perror("aio_read");
	    return 1;
	}
	
	/* Try to cancel the operation */
	int cancel_status = aio_cancel(fd, &aiocb);
	if (cancel_status == AIO_CANCELED) {
	    printf("Read operation was successfully canceled.\n");
	} else if (cancel_status == AIO_NOTCANCELED) {
	    printf("Read operation could not be canceled.\n");
	} else if (cancel_status == AIO_ALLDONE) {
	    printf("Read operation had already completed.\n");
	} else {
	    printf("Error canceling read operation: %d\n", errno);
	}
	close(fd);
	return 0;
}
```

It is worth noting that `aio_cancel()` only cancels operations that are in progress and it is not guaranteed that the operation will be canceled. Also, the aiocb struct that is passed as a parameter should have the operation (`aio_read`, `aio_write` or `aio_fsync`) already started.

