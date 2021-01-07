#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#define PROMPT "shell> "
#define MAX_BACKGROUND 50

void scanBackground(pid_t backArray[], int *backLen, char **cmdPtrs[], int *cmdPtrsLen);
void handleExit(char **command, pid_t backArray[], int *backLen, char **cmdPtrs[], int *cmdPtrsLen);
void handleJobs(char **command, pid_t backArray[], int *backLen);
void handleKill(char **command, pid_t backArray[], int *backLen, char **cmdPtrs[], int *cmdPtrsLen);
void foregroundProcess(char **command);
void backgroundProcess(char **command, pid_t backArray[], int *backLen, char **cmdPtrs[], int *cmdPtrsLen);

// Break input string into an array of strings.
// The last element of the array is set to NULL
char** tokenize(const char *input, const char *delimiters) {
    char *token = NULL;

    // make a copy of the input string, because strtok likes to mangle strings.  
    char *input_copy = strdup(input);

    // find out exactly how many tokens we have
    int count = 0;
    for (token = strtok(input_copy, delimiters); token; 
            token = strtok(NULL, delimiters)) {
        count++ ;
    }
    free(input_copy);

    input_copy = strdup(input);

    // allocate the array of char *'s, with one additional
    char **array = (char **)malloc(sizeof(char *)*(count+1));
    int i = 0;
    for (token = strtok(input_copy, delimiters); token;
            token = strtok(NULL, delimiters)) {
        array[i] = strdup(token);
        i++;
    }
    array[i] = NULL;
    free(input_copy);
    return array;
}

// Free all memory used to store an array of tokens.
void free_tokens(char **tokens) {
    int i = 0;
    while (tokens[i] != NULL) {
        free(tokens[i]); // free each string
        i++;
    }
    free(tokens); // then free the array
}



int main(int argc, char **argv) {
    printf("%s", PROMPT);
    fflush(stdout);  // Display the prompt immediately
    char buffer[1024];

    // Create the array of active background process
    pid_t backArray[MAX_BACKGROUND];
    // Keep track of its length with a pointer to an integer
    int backgroundArrayLen = 0;
    int *backLen = &backgroundArrayLen;
    // Create array to hold command tokens of background processes
    char **cmdPtrs[MAX_BACKGROUND];
    // Keep track of its length as well
    int commandPointersLength = 0;
    int *cmdPtrsLen = &commandPointersLength;

    
    while (fgets(buffer, 1024, stdin) != NULL) {
        char **command = tokenize(buffer, " \t\n");
        scanBackground(backArray, backLen, cmdPtrs, cmdPtrsLen);

        // RUN THE COMMAND:
        // Ignore empty commands
        if (command[0] == NULL) {
            printf("Command ignored\n");
            free_tokens(command);
        // Built-in exit command
        } else if (strcmp(command[0],"exit") == 0) {
            handleExit(command, backArray, backLen, cmdPtrs, cmdPtrsLen);
        // Built-in jobs command
        } else if (strcmp(command[0],"jobs") == 0) {
            handleJobs(command, backArray, backLen);
        // Built-in kill PID command
        } else if (strcmp(command[0],"kill") == 0) {
            handleKill(command, backArray, backLen, cmdPtrs, cmdPtrsLen);
        // Handle non-built-in commands
        } else {
            // For determining whether or not this command is a background command
            int background = 0;
            // Iterate through every token of the command
            int currentIdx = 0;
            while (command[currentIdx] != NULL) {
                // Detect &
                if (strcmp(command[currentIdx], "&") == 0) {
                    background = 1;
                }
                currentIdx++;
            }

            // Run process in foreground
            if (background == 0) {
                foregroundProcess(command);
                free_tokens(command);
            // Run process in background
            } else {
                backgroundProcess(command, backArray, backLen, cmdPtrs, cmdPtrsLen);
            }
            
        }

        printf("%s", PROMPT);
        fflush(stdout);  // Display the prompt immediately
    }

    return EXIT_SUCCESS;
}


// Scan the array of background processes and properly dispose of any processes that completed
void scanBackground(pid_t backArray[], int *backLen, char **cmdPtrs[], int *cmdPtrsLen) {
    // Store the finished processes in an array:
    pid_t completedPids[MAX_BACKGROUND];
    int numCompleted = 0;
    for (int i = 0; i < *backLen; i++) {
        int status = 0;
        pid_t childExitStatus = waitpid(backArray[i], &status, WNOHANG);
        // Check if child process has finished
        if (childExitStatus > 0) {
            printf("%d finished with exit code %d\n", backArray[i], status);
            // If so, add the child process PID to array of finished processes
            completedPids[numCompleted] = backArray[i];
            numCompleted++;
        }
    }
    // Remove the finished PIDs from the array of active background processes
    while (numCompleted != 0) {
        int seen = -1;
        for (int i = 0; i < *backLen; i++) {
            if (backArray[i] == completedPids[numCompleted-1]) {
                seen = i;
            }
            if (seen != -1) {
                backArray[i] = backArray[i+1];
            }
        }
        numCompleted = numCompleted - 1;
        *backLen = *backLen - 1;
        // Free the respective command tokens
        int ptrSeen = -1;
        for (int i = 0; i < *cmdPtrsLen; i++) {
            if (i == seen) {
                free_tokens(cmdPtrs[seen]);
                ptrSeen = 1;
            }
            if (ptrSeen == 1) {
                cmdPtrs[i] = cmdPtrs[i+1];
            }
        }
        *cmdPtrsLen = *cmdPtrsLen - 1;
    }
}

// Handle the built-in exit command (quit the shell)
void handleExit(char **command, pid_t backArray[], int *backLen, char **cmdPtrs[], int *cmdPtrsLen) {
    // Check that the correct syntax has been used
    if (command[1] != NULL) {
        printf("Too many arguments have been provided\n");
    }
    // Kill all background processes:
    for (int i = 0; i < *backLen; i++) {
        kill(backArray[i], SIGKILL);
    }
    printf("Exiting...\n");
    // Free all command tokens of background processes
    for (int i = 0; i < *cmdPtrsLen; i++) {
        free_tokens(cmdPtrs[i]);
    }
    // Free command tokens of the process that is current running and exit the process
    free_tokens(command);
    exit(EXIT_SUCCESS);
}

// Handle the built-in jobs command (print list of running background processes)
void handleJobs(char **command, pid_t backArray[], int *backLen) {
    // Check that the correct syntax has been used
    if (command[1] != NULL) {
        printf("Too many arguments have been provided\n");
    } else {
        // List background processes:
        printf("Process currently active: ");
        for (int i = 0; i < *backLen; i++) {
            if (i != (*backLen - 1)) {
                printf("%d, ", backArray[i]);
            } else {
                printf("%d", backArray[i]);
            }
        }
        printf("\n");
    }
    free_tokens(command);
}

// Handle the built-in kill <PID> command (kill specified process)
void handleKill(char **command, pid_t backArray[], int *backLen, char **cmdPtrs[], int *cmdPtrsLen) {
    // Check that correct syntax has been used
    if (command[1] == NULL) {
        printf("Must specify a PID\n");
    } else if (command[2] != NULL) {
        printf("Too many arguments have been provided\n");
    } else {
        // Check whether a process with the given pid exists
        int present = -1;
        for (int i = 0; i < *backLen; i++) {
            if (backArray[i] == atoi(command[1])) {
                present = i;
            }
        }
        if (present != -1) {
            // Check whether the child process is running
            pid_t childExitStatus = waitpid(atoi(command[1]), NULL, WNOHANG);
            // If the child process is done, waitpid will return the child's pid...
            if (childExitStatus > 0) {
                // Remove the target pid from the array
                int seen = -1;
                for (int i = 0; i < *backLen; i++) {
                    if (backArray[i] == atoi(command[1])) {
                        seen = i;
                    }
                    if (seen != 1) {
                        backArray[i] = backArray[i+1];
                    }
                }
                *backLen = *backLen - 1;
                // Free the respective command tokens
                int ptrSeen = -1;
                for (int i = 0; i < *cmdPtrsLen; i++) {
                    if (i == seen) {
                        free_tokens(cmdPtrs[seen]);
                        ptrSeen = 1;
                    }
                    if (ptrSeen == 1) {
                        cmdPtrs[i] = cmdPtrs[i+1];
                    }
                }
                *cmdPtrsLen = *cmdPtrsLen - 1;
                // In this case, the process had already completed but it's pid
                // had been erroneously left in the array of processes that are still
                // running.  As such, I remove it and carry on

            // If the child is not yet done, waitpid will return 0...
            } else if (childExitStatus == 0) {
                // Kill the child process
                kill(atoi(command[1]), SIGKILL);
                // Remove the target pid from the array
                int seen = -1;
                for (int i = 0; i < *backLen; i++) {
                    if (backArray[i] == atoi(command[1])) {
                        seen = i;
                    }
                    if (seen != -1) {
                        backArray[i] = backArray[i+1];
                    }
                }
                *backLen = *backLen - 1;
                // Free the target process's command tokens
                int ptrSeen = -1;
                for (int i = 0; i < *cmdPtrsLen; i++) {
                    if (i == seen) {
                        free_tokens(cmdPtrs[seen]);
                        ptrSeen = 1;
                    }
                    if (ptrSeen == 1) {
                        cmdPtrs[i] = cmdPtrs[i+1];
                    }
                }
                *cmdPtrsLen = *cmdPtrsLen - 1;
            // If there's an error with waitpid, it will return -1...
            } else {
                printf("The specified process could not be killed (there was an error with the waitpid() function\n");
            }
        } else {
            printf("There is no such process with the given PID\n");
        }
    }
    free_tokens(command);
}

// Handle non-built-in commands that occur in the foreground by invoking specified executable program
void foregroundProcess(char **command) {
    pid_t pid = fork();
    // Child is running
    if (pid == 0) {
        int programExitStatus = execv(command[0], command);
        if (programExitStatus == -1) {
            printf("Invoking the specified executable failed\n");
            exit(1);
        }
    // Parent is running
    } else {
        pid_t childExitStatus = waitpid(pid, NULL, 0);
        if (childExitStatus == -1) {
            printf("Invoking the specified executable failed\n");
        }
    }
    return;
}

// Handle non-built-in commands that occur in the background (indicated with &)
void backgroundProcess(char **command, pid_t backArray[], int *backLen, char **cmdPtrs[], int *cmdPtrsLen) {
    pid_t pid = fork();
    // Child is running
    if (pid == 0) {
        // Cannot call execv with & at the end
        int currentIdx = 0;
        while (command[currentIdx] != NULL) {
            if (strcmp(command[currentIdx], "&") == 0) {
                command[currentIdx] = NULL;
            }
            currentIdx++;
        }
        int programExitStatus = execv(command[0], command);
        // If the executable failed for some reason, clean up the process:
        programExitStatus = -1;
        printf("Invoking the specified executable in the background failed for pid %d\n", getpid());
        // Remove pid from array of incomplete background processes
        int seen = -1;
        for (int i = 0; i < *backLen; i++) {
            if (backArray[i] == atoi(command[1])) {
                seen = i;
                printf("pid detected!\n");
            }
            if (seen != -1) {
                backArray[i] = backArray[i+1];
            }
        }
        *backLen = *backLen - 1;
        // Must reinsert the &
        currentIdx = 0;
        while (command[currentIdx] != NULL) {
            currentIdx++;
        }
        command[currentIdx] = "&";
        command[currentIdx+1] = NULL;
        // Free command tokens of the child process
        int ptrSeen = -1;
        for (int i = 0; i < *cmdPtrsLen; i++) {
            if (i == seen) {
                free_tokens(cmdPtrs[seen]);
                ptrSeen = 1;
            }
            if (ptrSeen == 1) {
                cmdPtrs[i] = cmdPtrs[i+1];
            }
        }
        *cmdPtrsLen = *cmdPtrsLen - 1;
        exit(1);

    // Parent is running
    } else {
        // Record pid of child process in the array of unfinished background processes
        backArray[*backLen] = pid;
        *backLen = *backLen + 1;
        // Record command tokens ptr so that it can be freed later
        cmdPtrs[*cmdPtrsLen] = command;
        *cmdPtrsLen = *cmdPtrsLen + 1;
    }
    return;
}
