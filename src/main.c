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
#include <dirent.h>
#include <readline/readline.h>
#include <readline/history.h>

#define MAX_CMD_LEN 1024
#define MAX_ARGS 64
#define DELIMITERS " \t\r\n"

/**
 * Custom completer for files and directories.
 */
char *file_directory_completer(const char *text, int state) {
    static DIR *dir = NULL;
    static int len;
    struct dirent *entry;

    if (!state) {
        if (dir) {
            closedir(dir);
        }
        dir = opendir("."); // Open the current directory
        len = strlen(text);
    }

    while ((entry = readdir(dir)) != NULL) {
        if (strncmp(entry->d_name, text, len) == 0) {
            return strdup(entry->d_name);
        }
    }

    closedir(dir);
    dir = NULL;
    return NULL;
}

/**
 * Custom tab completion logic.
 */
char **shell_completion(const char *text, int start, int end) {
    rl_attempted_completion_over = 1; // Tell readline to use our function
    return rl_completion_matches(text, file_directory_completer);
}

/**
 * Changes the current working directory.
 */
void change_directory(const char *path) {
    if (path == NULL) {
        const struct passwd *pw = getpwuid(getuid());
        const char *homedir = pw->pw_dir;
        if (chdir(homedir) != 0) {
            perror("chdir");
        }
    } else {
        if (chdir(path) != 0) {
            perror("chdir");
        }
    }
}

/**
 * Executes a command.
 */
void execute_command(char *cmd) {
    char *argv[MAX_ARGS];
    int argc = 0;

    char *token = strtok(cmd, DELIMITERS);
    while (token != NULL) {
        argv[argc++] = token;
        token = strtok(NULL, DELIMITERS);
    }
    argv[argc] = NULL;

    if (argc > 0) {
        if (strcmp(argv[0], "cd") == 0) {
            change_directory(argc > 1 ? argv[1] : NULL);
            return;
        }
    }

    pid_t pid = fork();
    if (pid == 0) {
        execvp(argv[0], argv);
        perror("execvp");
        exit(EXIT_FAILURE);
    } else if (pid > 0) {
        wait(NULL);
    } else {
        perror("fork");
        exit(EXIT_FAILURE);
    }
}

/**
 * Main function of the simple shell.
 */
int main() {
    char *cmd;
    rl_attempted_completion_function = shell_completion;

    while (1) {
        cmd = readline("simple-shell> ");
        if (!cmd) break;

        if (strcmp(cmd, "clear") == 0 || strcmp(cmd, "ctrl+l") == 0) {
            system("clear");
            continue;
        }

        if (*cmd) {
            add_history(cmd);
            execute_command(cmd);
        }

        free(cmd);
    }

    return 0;
}

