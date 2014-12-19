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
    pid_t pid;
    signal(signo, SIG_IGN);
    while ((pid=wait(NULL)) < 0)
        if (errno != EINTR) return -1;
    
    printf("%d: finish\n", pid);

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
        case '<' : type = RE_IN; break;
        case '>' : type = RE_OUT; break;
		default : type = ARG;
			while ((*ptr != ' ') && (*ptr != '&') &&
				(*ptr != '\t') && (*ptr != '\0'))
				*tok++ = *ptr++;
	}
	*tok++ = '\0';
	return(type);
}

int execute(char **comm, int how, int re_in_index, int re_out_index)
{
	int	pid;
    int in_fd, out_fd;
    int pstat, wait_ret;

	if ((pid = fork()) < 0) {
		fprintf(stderr, "minish : fork error\n");
		return(-1);
	}
    else if (pid == 0) {
        if(re_in_index) {
            if((in_fd=open(comm[re_in_index], O_RDONLY)) < 0) {
                fprintf(stderr, "minish : file %s not found\n", comm[re_in_index]);
                exit(127);
            }
            else {
                dup2(in_fd, STDIN_FILENO);
            }

            comm[re_in_index] = (char*) '\0';
        }
        if(re_out_index) {
            if((out_fd = open(comm[re_out_index], O_WRONLY|O_CREAT|O_TRUNC, 0666)) < 0) {
                fprintf(stderr, "minish : file %d : %s create error\n",\
                        re_out_index, comm[re_out_index]);
                exit(127);
            }
            else {
                dup2(out_fd, STDOUT_FILENO);
            }
            comm[re_out_index] = (char*) '\0';
        } 

        execvp(*comm, comm);
        fprintf(stderr, "minish : command not found\n");
        exit(127);
    }
    
	if (how == BACKGROUND) {	// Background execution
        fprintf(stderr, "%d:%s\n", pid, *comm);
	}
	else {	// Foreground Execution
        while((wait_ret = waitpid(pid, &pstat, WNOHANG))==0);
        if(wait_ret < 0) {
            perror("wait err");
        }
	}
    

	return 0;
}

int parse_and_execute(char *input)
{
	char *arg[1024];
	int	type, how;
	int	quit = FALSE;
	int	narg = 0;
	int	finished = FALSE;
    int re_in_index = 0, re_out_index = 0;

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
					execute(arg, how, re_in_index, re_out_index);
			}
			narg = 0;
			if (type == EOL)
				finished = TRUE;
			break; 
        case RE_IN:
            re_in_index = narg;
            break;
        case RE_OUT:
            re_out_index = narg;
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


