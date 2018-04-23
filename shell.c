// #ifndef ARG_MAX
// #define ARG_MAX (sysconf(_sc_ARG_MAX))
// #endif

#include <fcntl.h>
#include <pwd.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <limits.h>

#include "history.h"
#include "timer.h"

#define HOST_NAME_MAX 64
#define ARG_MAX 1000

int linenum;

/* 
 * Input: Nothing
 * Output: Nothing
 * 
 * Purpose: Keep the program running
 */
int lush_loop();

/* 
 * Input: Token array
 * Output: Int 1, keep the program running
 * 
 * Purpose: Using chdir() to change CWD
 */
int lush_cd(char **args);

/* 
 * Input: Token array
 * Output: Nothing
 * 
 * Purpose: Handle more than one command
 */
void lush_pipes(char **args);

/* 
 * Input: User input line
 * Output: Tokens array
 * 
 * Purpose: Parser the line to tokens
 */
char** lush_parser(char *line);

/* 
 * Input: Token array
 * Output: Int 1, keep the program running
 * 
 * Purpose: Redirection to file, in and out
 */
int lush_re(char** args);

/* 
 * Input: Token array
 * Output: Boolean
 * 
 * Purpose: Determaind if the command need redirection
 */
bool lush_op(char** args);

/* 
 * Input: Single token
 * Output: Boolean
 * 
 * Purpose: Check if the token starts with #
 */
bool lush_start(char *str);

/* 
 * Input: Signo
 * Output: Nothing
 * 
 * Purpose: Ctrl + C check, not stop, not change the line number
 */
void sigint_handler(int signo);

/* 
 * Input: Token array
 * Output: Int 1, keep the program running
 * 
 * Purpose: Check if it is history execution
 */
int lush_hist(char **args);

bool lush_start(char *str) {

	return (strstr(str, "#") != NULL);
}

bool lush_start_hist(char *str) {

	return (strstr(str, "!") != NULL);
}

bool lush_op(char** args) {

	int i = 0;
	while (args[i] != NULL) {

		if (strcmp(args[i], "<") == 0 || strcmp(args[i], ">") == 0) {        
           return true;    
        }
        i++;
	}
	return false;
}

int lush_re(char** args) {

	int fd[2];
	int pid = fork();

	if (pid == 0) {
	    
	    char input[ARG_MAX];
	    char output[ARG_MAX];

	    int i = 0;
		while (args[i] != NULL) {

			if (strcmp(args[i], "<") == 0) {        
	            args[i] = NULL;
	            strcpy(input,args[i+1]);

	            if ((fd[0] = open(input, O_RDONLY, 0)) < 0) {
		            perror("lush");
		            return 1;
		        }           
		        dup2(fd[0], STDIN_FILENO); 

		        close(fd[0]);          
	        }               

		    if (strcmp(args[i], ">") == 0) {      
	            args[i] = NULL;
	            strcpy(output, args[i+1]);

	            if ((fd[1] = creat(output , 0644)) < 0) {
		            perror("lush");
		            return 1;
	        	}           
		        dup2(fd[1], STDOUT_FILENO);

		        close(fd[1]);
		    }
        	i++;
		}

	    execvp(*args, args);
	    perror("execvp");
	    _exit(0);
	} else if((pid) < 0) {     
	    
	    perror("lush");
	    return 1;
	} else {                                 
		int status;
	    while (!(wait(&status) == pid));
	}
	return 1;
}

void lush_getter() {

	char *name = NULL;
	char hostbuffer[HOST_NAME_MAX];
	char cwd[PATH_MAX];

	gethostname(hostbuffer, sizeof(hostbuffer));
    getcwd(cwd, sizeof(cwd));
    
    name = strrchr(cwd, '/');
    printf("[%d|%s@%s:~%s]$ ", linenum, getlogin(), hostbuffer, name);
    fflush(stdout);
}

char** lush_parser(char *line) {

	char **tokens = malloc(ARG_MAX * sizeof(char*));
	char *token = strtok(line, " \t\n");

	int i = 0;
    while (token != NULL) {
        tokens[i++] = token;
        token = strtok(NULL, " \t\n");
    }
    tokens[i] = NULL;

    i = 0;
    while (tokens[i] != NULL) {    		
    	if (lush_start(tokens[i]) == true) {
    		int j = i;
    		while (tokens[j] != NULL) {

    			tokens[j] = "\0";
    			j++;
    		}
    		break;
    	}

    	i++;
    }

    free(tokens);

    return tokens;

}

int lush_exec(char **args) {

	if (strcmp(args[0], "cd") == 0) {

		return lush_cd(args);
	} else if (strcmp(args[0], "history") == 0) {

		print_history();
		return 1;
	} else if (lush_op(args) == true) {

		lush_re(args);
	    return 1;

	} else if (lush_start_hist(args[0]) == true) {

		lush_hist(args);
		return 1;	
	} else {
		lush_pipes(args);

		return 1;
	}
}

int lush_hist(char **args) {

	if (linumer == 0) {

		printf("Nothing in history\n");
		return 1;
	}

	int last_index = -1;
	
	char* hist_exe = strchr(args[0], '!') + 1;
	if (strcmp(hist_exe, "!") == 0) {
		if (linumer > 0) {
			last_index = linumer - 1;
		}
	} else if (atoi(hist_exe) != 0) {

		last_index = atoi(hist_exe);
	} else {
		
		last_index = search_cmd(hist_exe);
	}

	if (last_index != -1 && last_index < linumer) {

		char buffer_tokens[ARG_MAX];
		char** buffer_buffer;
		strcpy(buffer_tokens, hist_str[last_index % HIST_MAX].cmd_name);
		buffer_buffer = lush_parser(buffer_tokens);
		lush_exec(buffer_buffer);	
	} else {

		perror("lush");
	}

	return 1;
}

int lush_cd(char **args) {

	if (args[1] == NULL || strcmp(args[1], "~") == 0) {
    	
    	struct passwd *pws;
		pws = getpwuid(geteuid());

    	chdir(pws->pw_dir);
  	} else {
    	if (chdir(args[1]) != 0) {
      		perror("lush");
    	}
  	}
	return 1;
}

/* NO THING PIPE HERE */
/* STUPIDEST PIPES IMPLEMENTATION */
/* I'm too stupid to figue thi out */
void lush_pipes(char **args) {

	int i = 0;
	char *next = NULL;
	
	while (args[i] != NULL) {

		if (strcmp(args[i], "|") == 0) {

			next = args[i+1];
			break;	
		}
		i++;
	}

	if (next ==  NULL) {
		pid_t pid = fork();
	    if (pid == 0) {
	        execvp(args[0], args);
	        printf("lush: no such file or directory: %s\n", args[0]);
	        _exit(0);
	    } else if (pid < 0) {

	    	perror("lush");
	    } else {
	        int status;
	        wait(&status);
	    }
	} 
	else {
		pid_t pid = fork();
	    int fd[2];
	    pipe(fd);

	    if (pid == 0) {
	        
	    	dup2(fd[1], STDOUT_FILENO);
	        close(fd[1]);
	        close(fd[0]);

	        execvp(args[0], args);

	        _exit(0);
	    } else if (pid < 0) {

	    	perror("lush");
	    } else {

	        dup2(fd[1], STDIN_FILENO);
	        lush_pipes(&next);
	        close(fd[0]);
	        close(fd[1]);

		}
	}
}

void sigint_handler(int signo) {
    
	printf("\n");
	fflush(stdout);
	lush_getter();
	fflush(stdout);
}

int lush_loop() {

	linenum = 0;
	int looper = 0;

	while (looper == 0) {

		char line[ARG_MAX];
		char buf[ARG_MAX];
        
        lush_getter();

        fgets(line, 1000, stdin);
        strcpy(buf, line);
        strtok(buf, "\n");

    	char **tokens = lush_parser(line);

    	if (tokens[0] != NULL) {

    		if (strcmp(tokens[0], "exit") == 0) {
	    		looper = 1;
	    		return 0;

	    	} else {
	    		double start = get_time();
	    		lush_exec(tokens);
	    		add_history(buf, get_time() - start);
	    		linenum++;
	    	}
    	}
    }
    return 0;
}

int main(void) {

	signal(SIGINT, sigint_handler);

	if (isatty(STDIN_FILENO)) {
        /* stdin is a TTY; entering interactive mode */
		lush_loop();
    } else {
        /*data piped in on stdin; entering script mode */
        char buffer[ARG_MAX];
        char* lines = fgets(buffer, ARG_MAX, stdin);

		buffer[strlen(buffer) - 1] = '\0';
		
		char** buffer_buffer;
		buffer_buffer = lush_parser(buffer);
		lush_exec(buffer_buffer);
    }

    return 0;
}
