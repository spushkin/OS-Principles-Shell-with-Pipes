/**************************************************************
* Class::  CSC-415-01 Summer 2024
* Name:: Siarhei Pushkin
* GitHub-Name:: spushkin
* Project:: Assignment 3 – Simple Shell with Pipes
*
* File:: Pushkin_Siarhei_HW3_main.c
*
* Description:: A simple shell implementation in C that supports
*               pipes and allows users to execute commands.
**************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define BUFFER_SIZE 163

// splits input string into arguments, handles quoted strings
int splitInput(char *input, char **args) {
    int argsCount = 0;
    char *token;
    int inQuote = 0;

    while (*input) {

        while (*input && strchr(" \t\n", *input)) input++;

        // toggle flag on encountering a quote
        if (*input == '"') {
            inQuote = !inQuote;
            input++;
        }

        // split input by whitespace or until closing quote
        if (!inQuote) {
            token = strsep(&input, " \t\n");
        } else {
            token = input;
            while (*input && (*input != '"')) input++;
            if (*input) *input++ = '\0';
        }

        // add token to arguments array if not empty
        if (*token) {
            args[argsCount++] = token;
            if (argsCount >= 4) break;
        }
    }
    args[argsCount] = NULL;
    return argsCount;
}

// forks a new process to execute a command
void executeCommand(char **args) {
    pid_t pid = fork();
    int status;

    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        execvp(args[0], args);
        perror("execvp");
        exit(EXIT_FAILURE);
    } else {
        if (waitpid(pid, &status, 0) == -1) {
            perror("waitpid");
        } else {
            printf("Child %d exited with %d\n", pid, WEXITSTATUS(status));
        }
    }
}

// handles multiple piped commands
void handlePipes(char *input) {
    char *commands[10];
    int commandCount = 0;
    char *command = strtok(input, "|");

    // split input into separate commands by pipe character
    while (command != NULL && commandCount < 9) {
        commands[commandCount++] = command;
        command = strtok(NULL, "|");
    }
    commands[commandCount] = NULL;

    int in_fd = 0;
    int status;
    pid_t pid;

    // iterate over each command to set up piping
    for (int i = 0; i < commandCount; i++) {
        char *args[5];
        splitInput(commands[i], args);

        int pipefd[2];
        if (i < commandCount - 1) {
            if (pipe(pipefd) == -1) {
                perror("pipe");
                exit(EXIT_FAILURE);
            }
        }

        pid = fork();
        if (pid == -1) {
            perror("fork");
            exit(EXIT_FAILURE);
        } else if (pid == 0) {
            if (in_fd != 0) {
                dup2(in_fd, STDIN_FILENO);
                close(in_fd);
            }
            if (i < commandCount - 1) {
                dup2(pipefd[1], STDOUT_FILENO);
                close(pipefd[1]);
            }
            execvp(args[0], args);
            perror("execvp");
            exit(EXIT_FAILURE);
        } else {
             // close input file descriptor in parent
            if (in_fd != 0) close(in_fd);
             // close output file descriptor in parent
            if (i < commandCount - 1) close(pipefd[1]);
            // save input file descriptor for next command
            in_fd = pipefd[0];
        }
    }

    // wait for all child processes to finish
    for (int i = 0; i < commandCount; i++) {
        pid = wait(&status);
        printf("Child %d exited with %d\n", pid, WEXITSTATUS(status));
    }
}

int main(int argc, char *argv[]) {
    char *inpBuffer = NULL;
    size_t bufferSize = BUFFER_SIZE;
    char *args[5];
    char *prompt = (argc > 1) ? argv[1] : "> ";

    inpBuffer = (char *)malloc(bufferSize * sizeof(char));
    if (inpBuffer == NULL) {
        perror("Unable to allocate buffer");
        exit(EXIT_FAILURE);
    }

    // main loop to read and process user input
    while (1) {
        if (isatty(STDIN_FILENO)) {
            printf("%s ", prompt);
        }

        if (getline(&inpBuffer, &bufferSize, stdin) == -1) {
            if (feof(stdin)) {

                printf("\n");
                break;
            } else {

                perror("getline");
                free(inpBuffer);
                exit(EXIT_FAILURE);
            }
        }

        if (inpBuffer[0] == '\n' || strchr(inpBuffer, '\n') == inpBuffer) {
            printf("Please, enter your command!\n");
            continue;
        }

        if (strchr(inpBuffer, '|')) {
            handlePipes(inpBuffer);
        } else {
            int argsCount = splitInput(inpBuffer, args);

            if (argsCount == 0) {
                printf("Please, enter your command!\n");
                continue;
            }

            if (strcmp(args[0], "exit") == 0) {
                break;
            }

            executeCommand(args);
        }
    }

    free(inpBuffer);
    return 0;
}
