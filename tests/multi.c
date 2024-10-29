#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <test1> <test2>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    pid_t pid1, pid2;

    // Define the command and arguments for execvp
    char *command = "../client/c"; 

    // Construct the full paths for the test files
    char test1[256];
    char test2[256];
    snprintf(test1, sizeof(test1), "../tests/%s.txt", argv[1]);
    snprintf(test2, sizeof(test2), "../tests/%s.txt", argv[2]);

    char *args1[] = {"./c", "-t", test1, NULL}; // Arguments for the first client process
    char *args2[] = {"./c", "-t", test2, NULL}; // Arguments for the second client process

    pid1 = fork();
    if (pid1 == 0) {  // Child process 1
        execvp(command, args1);
        perror("Error executing execvp for Client 1"); // If execvp fails
        exit(EXIT_FAILURE);
    }

    pid2 = fork();
    if (pid2 == 0) {  // Child process 2
        execvp(command, args2);
        perror("Error executing execvp for Client 2"); // If execvp fails
        exit(EXIT_FAILURE);
    }

    // Wait for both child processes to finish
    waitpid(pid1, NULL, 0);
    waitpid(pid2, NULL, 0);

    return 0;
}