#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>

#include <signal.h>

#define FALSE 0
#define TRUE 1

#define EOL	1
#define ARG	2
#define AMPERSAND 3
#define RE_IN 4
#define RE_OUT 5

#define FOREGROUND 0
#define BACKGROUND 1

static char	input[512];
static char	tokens[1024];
char		*ptr, *tok;

int sigchild_handler(int signo) 
{
    //pid_t pid;
//    signal(signo, SIG_IGN);
    while ((pid=wait(NULL)) < 0)
        if (errno != EINTR) return -1;
    
    //printf("pid : %d exit\n", pid);
    return 0;
}

int get_token(char **outptr)
{
	int	type;

	*outptr = tok;
	while ((*ptr == ' ') || (*ptr == '\t')) ptr++;

	*tok++ = *ptr;

	switch (*ptr++) {
		case '\0' : type = EOL; break;
		case '&': type = AMPERSAND; break;
        case '<' : type = RE_IN break;
        case '>' : type = RE_OUT break;
		default : type = ARG;
			while ((*ptr != ' ') && (*ptr != '&') &&
				(*ptr != '\t') && (*ptr != '\0'))
				*tok++ = *ptr++;
	}
	*tok++ = '\0';
	return(type);
}

int execute(char **comm, int how)
{
	int	pid;

	if ((pid = fork()) < 0) {
		fprintf(stderr, "minish : fork error\n");
		return(-1);
	}
	else if (pid == 0) {
		execvp(*comm, comm);
		fprintf(stderr, "minish : command not found\n");
		exit(127);
	}

    if (how != BACKGROUND) {
        pause();
    }
    /*
	if (how == BACKGROUND) {	// Background execution

	}
	else {	// Foreground Execution
        pause();
	}
    */

	return 0;
}

int parse_and_execute(char *input)
{
	char *arg[1024];
	int	type, how;
	int	quit = FALSE;
	int	narg = 0;
	int	finished = FALSE;

	ptr = input;
	tok = tokens;
	while (!finished) {
		switch (type = get_token(&arg[narg])) {
		case ARG :
			narg++;
			break;
		case EOL :
		case AMPERSAND:
			if (!strcmp(arg[0], "quit")) quit = TRUE;
			else if (!strcmp(arg[0], "exit")) quit = TRUE;
			else if (!strcmp(arg[0], "cd")) {
                chdir(arg[1]);
			}
			else if (!strcmp(arg[0], "type")) {
				if (narg > 1) {
					int	i, fid;
					int	readcount;
					char	buf[512];
					/*  학생들이 프로그램 작성할 것
					fid = open(arg[1], ...);
					if (fid >= 0) {
						readcount = read(fid, buf, 512);
						while (readcount > 0) {
							for (i = 0; i < readcount; i++)
								putchar(buf[i]);
							readcount = read(fid, buf, 512);
						}
					}
					close(fid);
					*/
				}
			}
			else {
				how = (type == AMPERSAND) ? BACKGROUND : FOREGROUND;
				arg[narg] = NULL;
				if (narg != 0)
					execute(arg, how);
			}
			narg = 0;
			if (type == EOL)
				finished = TRUE;
			break; 
		}
	}
	return quit;
}



main()
{
	int	quit;
    char cwd[1024];
	printf("msh(%s) # ", getcwd(cwd, 1024));
 
    signal(SIGCHLD, sigchild_handler);

	while (gets(input)) {
		quit = parse_and_execute(input);
		if (quit) break;
        printf("msh(%s) # ", getcwd(cwd, 1024));
	}
}


