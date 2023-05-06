# Narzędzia

## Waiting for a signal
```c
sigset_t set;
sigemptyset(&set);
sigaddset(&set, SIGINT);
if (sigprocmask(SIG_BLOCK, &set, NULL) == -1) {
    perror("sigprocmask");
    exit(EXIT_FAILURE);
}
// Wait for SIGINT signal
int sig;
sigwait(&set, &sig);
```

## Reading input
```c
static char stdin_buff[MSG_SIZE];
while (fgets(stdin_buff, MSG_SIZE, stdin) != NULL) {

    char* endptr;
    errno = 0;
    uint32_t msg = strtol(stdin_buff, &endptr, 10);
    if (errno != 0) {
        perror("strtol");
        exit(EXIT_FAILURE);
    }

    if (endptr == stdin_buff) { // it is not a number
        continue;
    }

    // do something with the number
}
```