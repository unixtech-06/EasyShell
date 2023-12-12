/*	$Project: EasyShell, Version: 2023, Last Maintenance: 2023/12/31 14:31 $	*/
/*
 * Copyright (c) 2023, Ryosuke
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * File: main.c
 */

#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
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