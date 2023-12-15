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

#include <sys/types.h>  /* Non-local includes in brackets. */
#include <sys/wait.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pwd.h>
#include <dirent.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <err.h>

#define MAX_CMD_LEN 1024
#define MAX_ARGS 64
#define DELIMITERS " \t\r\n"

/*
 * Retrieves a list of commands available in the system PATH.
 * This function searches each directory in the PATH environment variable
 * for executable files, which are potential commands.
 *
 * Parameters:
 *   text - The initial text to match against command names.
 *   list_size - Pointer to an integer where the size of the returned list will be stored.
 *
 * Returns:
 *   A dynamically allocated array of strings, each representing a command name
 *   that starts with the provided text. The size of the array is stored in list_size.
 *   If no commands are found, returns NULL.
 */
	char **
get_commands_from_path(const char *text, int *list_size)
{
	/* Duplicate the PATH environment variable to avoid modifying the original. */
	const char *path = getenv("PATH");
	char *path_copy = strdup(path);
	char **command_list = NULL;
	int count = 0;

	/* Tokenize the PATH string to iterate over each directory. */
	char *dir_path = strtok(path_copy, ":");
	while (dir_path != NULL) {
		DIR *dir = opendir(dir_path);
		if (dir != NULL) {
			struct dirent *entry;
			while ((entry = readdir(dir)) != NULL) {
				struct stat st;
				char full_path[1024];

				/* Construct the full path of the file and check if it's executable. */
				snprintf(full_path, sizeof(full_path), "%s/%s", dir_path, entry->d_name);
				if (stat(full_path, &st) == 0 && (st.st_mode & S_IXUSR)) {
					/* If the file name matches the provided text, add it to the list. */
					if (strncmp(entry->d_name, text, strlen(text)) == 0) {
						command_list = realloc(command_list, sizeof(char *) * (count + 1));
						command_list[count++] = strdup(entry->d_name);
					}
				}
			}
			closedir(dir);
		}
		dir_path = strtok(NULL, ":");
	}
	free(path_copy);
	*list_size = count;
	return command_list;
}

/* Custom completer for files and directories. */
	char *
file_directory_completer(const char *text, int state)
{
	static DIR *dir = NULL;
	static int len;
	struct dirent *entry;

	if (!state) {
		if (dir)
			closedir(dir);
		dir = opendir("."); /* Open the current directory */
		len = strlen(text);
	}

	while ((entry = readdir(dir)) != NULL) {
		if (strncmp(entry->d_name, text, len) == 0)
			return strdup(entry->d_name);
	}

	closedir(dir);
	dir = NULL;
	return NULL;
}

/* Custom tab completion logic. */
	char **
shell_completion(const char *text, int start, int end)
{
	rl_attempted_completion_over = 1;

	if (start == 0) {
		int list_size = 0;
		char **commands = get_commands_from_path(text, &list_size);
		if (list_size > 0) {
			return commands;
		}
	}

	return rl_completion_matches(text, file_directory_completer);
}

/* Changes the current working directory. */
	void 
change_directory(const char *path) 
{
	if (path == NULL) {
		const struct passwd *pw = getpwuid(getuid());
		const char *homedir = pw->pw_dir;
		if (chdir(homedir) != 0)
			err(1, "chdir failed to home directory");
	} else {
		if (chdir(path) != 0)
			warn("chdir failed to %s", path);
	}
}

/* Executes a command. */
	void
execute_command(char *cmd) 
{
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

	const pid_t pid = fork();
	if (pid == 0) {
		execvp(argv[0], argv);
		err(1, "execvp failed on command %s", argv[0]);
	} else if (pid > 0) {
		if (wait(NULL) == -1)
			warn("wait failed");
	} else {
		err(1, "fork failed");
	}
}

/* Main function of the simple shell. */
	int
main(void)
{

	rl_attempted_completion_function = shell_completion;

	while (1) {
		char *cmd = readline("simple-shell> ");
		if (!cmd)
			break;

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
