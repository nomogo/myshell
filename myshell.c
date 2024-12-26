#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>

#define MAX_LINE 256
#define MAX_ARGS 32

void handle_sighup(int sig) {
    printf("Configuration rechargée\n");
}

char** parse_command(char* line, int *arg_count) {
    char** args = malloc(sizeof(char*) * MAX_ARGS);
    if(args == NULL) {
        perror("malloc failed");
        return NULL;
    }
    char *token = strtok(line, " ");
    int i = 0;
    while(token != NULL && i < MAX_ARGS - 1) {
        args[i] = token;
        token = strtok(NULL, " ");
        i++;
    }
    args[i] = NULL;
    *arg_count = i;
    return args;
}

void execute_command(char** args) {
    pid_t pid = fork();
    if (pid == 0) {
        execvp(args[0], args);
        perror("execvp failed");
        exit(EXIT_FAILURE);
    } else if (pid > 0) {
        int status;
        if (waitpid(pid, &status, 0) == -1) {
            perror("waitpid failed");
        } else {
          if (WIFEXITED(status)) {
            if(WEXITSTATUS(status) != 0) {
              fprintf(stderr, "Command exited with status: %d\n", WEXITSTATUS(status));
            }
          }
          else if (WIFSIGNALED(status)) {
                fprintf(stderr, "Command terminated by signal: %d\n", WTERMSIG(status));
          }
        }
    } else {
        perror("fork failed");
    }
}

int main() {
    char line[MAX_LINE];
    signal(SIGHUP, handle_sighup);

    while (1) { // Main loop
        printf("> "); // Display prompt
        if (fgets(line, sizeof(line), stdin) == NULL) {
           if(feof(stdin)){ // Ctrl+D
             printf("\nExiting myshell...\n");
           }
           else { // Some other error, handle it
              perror("fgets failed");
           }
           break;
        }
        line[strcspn(line, "\n")] = 0; // Remove trailing newline

        if (strcmp(line, "exit") == 0 || strcmp(line, "\\q") == 0) {
            printf("Exiting myshell...\n");
            break;
        }

        int arg_count;
        char** args = parse_command(line, &arg_count);
        if(args == NULL) {
            continue;
        }

        if(arg_count > 0) {
            if(strcmp(args[0], "echo") == 0) {
                // Handle echo separately as a built-in
                for(int i=1; i < arg_count; i++){
                  printf("%s%s", args[i], (i == arg_count-1) ? "" : " ");
                }
                printf("\n");

            } else {
               execute_command(args);
            }
        }
        free(args);
    }
    return 0;
}
