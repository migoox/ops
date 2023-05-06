# Lab 2


## fork() and exec() functions

fork() function is used to create a new process. This function creates a copy of the calling process, which is called the child process. The child process is an exact duplicate of the parent process, with its own separate address space and set of system resources.

The fork() function returns a value to the calling process to indicate the status of the child process.

```c
pid_t pid = fork();
if (pid == 0) {
  // This is the child process
  // Do some work here
  exit(0);
} else if (pid > 0) {
  // This is the parent process
  // Do some work here
  waitpid(pid, &status, 0);
} else {
  // The fork() function failed
  perror("Fork failed");
  exit(EXIT_FAILURE);
}
```

The exec() function is used to execute a new program in the context of an existing process. It replaces the current program in the process with the specified program, and it loads the new program into the process's address space. The exec() function does not create a new process, but it does create a new instance of the specified program within the existing process.

In general, the fork() and exec() functions are used together in programs that need to create and run new processes.

## waitpid() and wait() functions

The waitpid() function is used to wait for a child process to finish executing. This function is typically called by the parent process of the child process, and it causes the parent process to pause until the child process has terminated.

In the example above, the waitpid() function is called by the parent process after the child process has been created using the fork() function. The waitpid() function causes the parent process to pause until the child process has terminated, at which point the parent process can continue executing.

The most important difference is that the wait() function waits for any child process to finish, while the waitpid() function allows the parent process to specify which child process it wants to wait for.

```c
pid_t pid1 = fork();
if (pid1 == 0) {
  // This is the first child process
  // Do some work here
  exit(0);
} else if (pid1 > 0) {
  pid_t pid2 = fork();
  if (pid2 == 0) {
    // This is the second child process
    // Do some work here
    exit(0);
  } else if (pid2 > 0) {
    // This is the parent process
    // Wait for the first child process to finish
    wait(NULL);
    // Wait for the second child process to finish
    waitpid(pid2, &status, 0);
    // Do some work here
  } else {
    // The fork() function failed
    perror("Fork failed");
    exit(1);
  }
} else {
  // The fork() function failed
  perror("Fork failed");
  exit(1);
}

```

## Zombies and orphans

A **zombie** (also known as a "defunct" process) is a process that has terminated, but whose parent process has not yet waited for it to finish. This can happen if 
- the parent process terminates before the child process, 
- or if the parent process does not call the wait() or waitpid() function to wait for the child process to finish.

When a child process becomes a zombie, it remains in the process table, but it is no longer running and it cannot be interacted with. 

To remove a zombie process, the parent process of the zombie must call the wait() or waitpid() function to wait for the zombie process to finish. This will cause the zombie process to be removed from the process table and its resources to be released.

You can send SIGCHLD to the parent process in order to get rid of the zombie.

An **orphan process**, on the other hand, is a process whose parent process has terminated. When a parent process terminates, any child processes that it has created are automatically reparented to the init process, which is the parent of all processes on the system. The init process is responsible for cleaning up orphaned processes, so they do not become zombies.

## Process signals

```bash
# display all process signals
$ kill -l
```

Some of the most common process signals include:
- **SIGINT**: This signal is sent to a process when the user types the interrupt key (usually Ctrl+C) on the keyboard. It is used to interrupt a running process, and the default behavior is to terminate the process.

- **SIGQUIT**: This signal is sent to a process when the user types the quit key (usually Ctrl+\ ) on the keyboard. It is similar to SIGINT, but it is more severe, and the default behavior is to terminate the process and dump a core file (i.e., a file containing information about the state of the process when it was terminated).

- **SIGTSTP**: This signal is sent to a process when the user types the suspend key (usually Ctrl+Z) on the keyboard. It is used to suspend a running process, and the default behavior is to stop the process until it receives a SIGCONT signal.

- **SIGCONT**: This signal is sent to a process when it has been previously suspended with a SIGTSTP signal, in order to resume its execution.

- **SIGKILL**: This signal is sent to a process when it is necessary to terminate the process immediately, without giving it a chance to clean up or save its state. This is the most severe signal, and the process cannot ignore or catch it.

## Session vs group of processes

A group of processes is simply a collection of processes that have been grouped together for some reason. This could be because the processes are related to each other, or because they need to be managed together in some way.

A session is a collection of processes associated with a particular terminal. Session has its session leader.

There are several key differences between a group of processes and a session. For example:

- a group of processes can consist of processes from different sessions, while a session can only contain processes that are associated with that particular terminal.
- a group of processes is typically managed by the user or by another process, while a session is managed by the init process.
- a group of processes can be created and destroyed at any time, while a session is created when the user logs in and is destroyed when the user logs out.

## Foreground job vs background job

Foreground job is a process that is currently running and is associated with the terminal, meaning that it is currently interacting with the user.

Background job is a process that is not associated with the terminal and is not currently interacting with the user.

- Foreground jobs can receive input from the user and display output on the screen, while background jobs cannot.
- Foreground jobs can be suspended or terminated by the user using shortcut keys, while background jobs cannot.
- Foreground jobs are associated with the terminal and can be managed using the command line interface, while background jobs are not associated with the terminal and must be managed using other tools.

The SIGTTIN signal is a process signal that is sent to the process group of a background process when it attempts to read from its controlling terminal. This normally causes all of the processes in that group to stop (unless they handle the signal and don’t stop themselves). However, if the foreground process that attempts to read is ignoring or blocking this signal, then read fails with an EIO error instead. 

Similarly, when a process in a background job tries to write to its controlling terminal, the default behavior is to send a SIGTTOU signal to the process group. However, the behavior is modified by the TOSTOP bit of the local modes flags. If this bit is not set (which is the default), then writing to the controlling terminal is always permitted without sending a signal. Writing is also permitted if the SIGTTOU signal is being ignored or blocked by the writing process. 

## Orphaned Process Groups

To prevent problems, process groups that continue running even after the session leader has terminated are marked as orphaned process groups. 


## Initializing the shell

If the subshell is not running as a foreground job, it must stop itself by sending a SIGTTIN signal to its own process group. It may not arbitrarily put itself into the foreground; it must wait for the user to tell the parent shell to do this. If the subshell is continued again, it should repeat the check and stop itself again if it is still not in the foreground. 

Once the subshell has been placed into the foreground by its parent shell, it can enable its own job control. It does this by calling setpgid to put itself into its own process group, and then calling tcsetpgrp to place this process group into the foreground. 

Once the shell has taken responsibility for performing job control on its controlling terminal, it can launch jobs in response to commands typed by the user.

When a foreground process is launched, the shell must block until all of the processes in that job have either terminated or stopped. It can do this by calling the waitpid function

