/* Hubert Obrzut,
   Lista nr 3 
   Powłoka systemowa z biblioteką readline,
   przekierowaniami i autouzupełnieniami.*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <readline/readline.h>
#include <readline/history.h>

void login();
char* get_prompt();
char* read_line();
char** parse_line(char*, char**, int*);
void trim(char*);

int exec_cd	  (char**);
int exec_exit (char**);
int exec_help (char**);

int builtin_num();
int launch(char**, char*, int);
int execute(char**, char*, int);

struct builtin
{
	int (*func)(char**);
	char* name;
} builtins[] = {
	{ &exec_cd, "cd" },
	{ &exec_exit, "exit" },
	{ &exec_help, "help" }
};

char* username;
char* hostname;

#define NONE 0
#define REDIRECT_IN 1
#define REDIRECT_OUT 2
#define ASYNC 4

void print_args(char** args, char* redirect, int flag)
{
	while (*args != NULL)
		printf("arg: %s\n", *args++);
	printf("redirect: %s\n", redirect);
	printf("flag: %d\n", flag);
}

int main()
{
	login();

	int status = 1, flag;
	char* line;
	char* redirect = NULL;
	char** args;

	while (status)
	{
		line = read_line();
		args = parse_line(line, &redirect, &flag);
		// print_args(args, redirect, flag);
		status = execute(args, redirect, flag);

		free(line);
		free(args);
	}

	free(hostname);

	return EXIT_SUCCESS;
}

void login()
{
	username = getenv("USER");
	hostname = malloc(64 * sizeof(char));
	gethostname(hostname, 64);

	printf("logged in as %s\n", username);
	printf("print \"help\" to get more information\n\n");
}

char* get_prompt()
{
	static char path[1000];
	getcwd(path, sizeof(path));

	char* dir = path + strlen(path);
	while ( *(dir - 1) != '/')
		--dir;

	char* prompt;
	asprintf(&prompt, "\e[31m[\e[33m%s\e[32m@\e[34m%s\e[35m %s\e[31m]\e[m$ ", username, hostname, dir);
	return prompt;
}

char* read_line()
{
	char* prompt = get_prompt();
	char* line = readline(prompt);
	free(prompt);
	if (strlen(line) > 0)
		add_history(line);
	return line;
}

void trim(char* word)
{
	if (word == NULL)
		return;
	int begin = 0, end = strlen(word) - 1;
	while (isspace(word[begin]))
		++begin;
	while (end >= begin && isspace(word[end]))
		--end;
	for (int i = begin; i <= end; ++i)
		word[i - begin] = word[i];
	word[end + 1 - begin] = '\0';
}

#include <assert.h>
char** parse_line(char* line, char** redirect, int* flag)
{
	*flag = NONE;
	char* tmp = line;
	strsep(&tmp, ">");
	if (tmp != NULL)
	{
		*flag = REDIRECT_OUT;
		*redirect = tmp;
	}
	else
	{
		tmp = line;
		strsep(&tmp, "<");
		if (tmp != NULL)
		{
			*flag = REDIRECT_IN;
			*redirect = tmp;
		}
	}

	trim(*redirect);

	static const int BUFFSIZE = 64;
	static const char* delims = " \t\r\n\a";

	int buffsize = BUFFSIZE, it = 0;
	char** args = malloc(buffsize * sizeof(char*));

	char* token = strtok(line, delims);
	while (token != NULL)
	{
		args[it++] = token;
		if (it >= buffsize)
		{
			buffsize += BUFFSIZE;
			args = realloc(args, buffsize * sizeof(char*));
		}
		token = strtok(NULL, delims);
	}

	args[it] = NULL;
	return args;
}

int exec_cd(char** args)
{
	char* arg = args[1];
	if (arg == NULL)
		asprintf(&arg, "/home/%s", username);
	if (chdir(arg) != 0)
		perror("shell");
	return 1;
}

int exec_exit(char** args)
{
	return 0;
}

int exec_help(char** args)
{
	printf("Custom shell terminal.\n");
	printf("The following are the built in:\n");
	for (int i = 0; i < builtin_num(); ++i)
		printf("   %s\n", builtins[i].name);
	printf("Use system manual (man) to get more information about particular command.\n");
	return 1;
}

int builtin_num()
{
	return sizeof(builtins) / sizeof(struct builtin);
}

int launch(char** args, char* redirect, int flag)
{
	pid_t pid = fork();
	if (pid == 0)
	{
		int fd;
		switch (flag)
		{
			case REDIRECT_IN:
				fd = open(redirect, O_RDONLY | O_CREAT, 0777);
				dup2(fd, STDIN_FILENO);
				break;

			case REDIRECT_OUT:
				fd = open(redirect, O_WRONLY | O_CREAT, 0777);
				dup2(fd, STDOUT_FILENO);
				break;
		}

		if (execvp(args[0], args) == -1)
			perror("shell");
		exit(EXIT_FAILURE);
	}
	if (pid > 0)
	{
		int status;
		do {
			waitpid(pid, &status, WUNTRACED);
		} while (!WIFEXITED(status) && !WIFSIGNALED(status));
	}
	else
		perror("shell");

	return 1;
}

int execute(char** args, char* redirect, int flag)
{	
	if (args[0] == NULL)
		return 1;
	for (int i = 0; i < builtin_num(); ++i)
		if (strcmp(args[0], builtins[i].name) == 0)
			return (*builtins[i].func)(args);
	return launch(args, redirect, flag);
}
