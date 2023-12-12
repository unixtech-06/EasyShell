#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <pwd.h>

#define MAX_CMD_LEN 1024
#define MAX_ARGS 64
#define DELIMITERS " \t\r\n"

/**
 * Changes the current working directory.
 *
 * @param path The path to change to. If NULL, changes to the home directory.
 */
void
change_directory(const char *path)
{
    if (path == NULL) {
        // Change to the home directory
        const struct passwd *pw = getpwuid(getuid());
        const char *homedir = pw->pw_dir;
        if (chdir(homedir) != 0) {
            perror("chdir");
        }
    } else {
        // Change to the specified directory
        if (chdir(path) != 0) {
            perror("chdir");
        }
    }
}

/**
 * Executes a command by creating a child process or handling built-in commands.
 *
 * @param cmd The command to be executed.
 */
void
execute_command(char *cmd)
{
    char *argv[MAX_ARGS];
    int argc = 0;

    // Split the command into tokens to create an argument array
    char *token = strtok(cmd, DELIMITERS);
    while (token != NULL) {
        argv[argc++] = token;
        token = strtok(NULL, DELIMITERS);
    }
    argv[argc] = NULL; // Terminate the argument array with NULL

    // Handle built-in commands
    if (argc > 0) {
        if (strcmp(argv[0], "cd") == 0) {
            change_directory(argc > 1 ? argv[1] : NULL);
            return;
        }
    }

    // Create a child process to execute the command
    const pid_t pid = fork();
    if (pid == 0) {
        // Child process
        execvp(argv[0], argv);
        // If execvp fails
        perror("execvp");
        exit(EXIT_FAILURE);
    } else if (pid > 0) {
        // Parent process
        wait(NULL); // Wait for the child process to finish
    } else {
        // Fork failed
        perror("fork");
        exit(EXIT_FAILURE);
    }
}

/**
 * Main function of the simple shell.
 * It continuously prompts the user to enter commands and executes them.
 */
int
main()
{

    while (1) {char cmd[MAX_CMD_LEN];
        printf("simple-shell> "); // Display the prompt
        if (fgets(cmd, sizeof(cmd), stdin) == NULL) {
            break; // Exit the loop if input has ended
        }

        if (cmd[0] == '\n') {
            continue; // Ignore empty lines
        }

        execute_command(cmd);
    }

    return 0;
}