#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// program will display all environment strings

// extern (external) keyword
// - extends visibility of C variables and C functions
// - declaration - saying that something exists somewhere in the program
// - definition - means allocating memory for this func or variable
// 
// extern int a; -> declaration
// int a; -> declaration and definition
// 
// - it says to compiler "go outside my scope and find the definition"
// - if such a definition doesn't exist, linker will throw an error

extern char **environ;
// the variable environ points to an array of pointers to string called
// the "environment". The last pointer in this array has the value NULL
int main(int argc, char **argv)
{
    int i = 0;
    while(environ[i] != NULL)
    {
        printf("[%i]: %s\n", i, environ[i]);
        i++;
    }
    // you can add another environmental variable in bash before 
    // running the program
    // it will be added but not remembered in the shell
    // $ YOURVAR="my var" ./prog7

    // to force shell to remember the environmental variable
    // you have to export it
    // $ export YOURVAR="my var"
    // $ ./prog7

    // warning: newly added exported variable's index in
    // environ is undefined

    // warning2: if new shell (another terminal) will be opened
    // exported value won't be valid in this shell

    // warning3: if new shell will be added from the shell we 
    // exported new variable in, the new one will inherit all of the
    // environmental variables including it

    return EXIT_SUCCESS;
}