#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include <signal.h>

#include "re_pi_info.h"

#define FALSE 0
#define TRUE 1

#define EOL	1
#define ARG	2
#define AMPERSAND 3
#define RE_IN 4
#define RE_OUT 5
#define PIPE 6

#define FOREGROUND 0
#define BACKGROUND 1

static char	input[512];
static char	tokens[1024];

static int pipes[512][2];

static int_node *bg_proc_head;

char		*ptr, *tok;

void sigchild_handler(int signo) 
{
    pid_t pid;
    //signal(signo, SIG_IGN);
    while ((pid=wait(NULL)) < 0)
        if (errno != EINTR) return ;
    
    //printf("%d: finish\n", pid);

    return ;
}

void sig_handler(int signo) 
{
    switch(signo) {
        case SIGINT:
            fprintf(stderr, "\b\b  \b\b");
            break;
    }
}

int get_token(char **outptr)
{
	int	type;

	*outptr = tok;
	while ((*ptr == ' ') || (*ptr == '\t')) ptr++;

	*tok++ = *ptr;
    //putchar(*ptr);putchar('\n');
	switch (*ptr++) {
		case '\0' : type = EOL; break;
		case '&': type = AMPERSAND; break;
        case '<' : type = RE_IN; break;
        case '>' : type = RE_OUT; break;
        case '|' : type = PIPE; break;
		default : type = ARG;
			while ((*ptr != ' ') && (*ptr != '&') &&
				(*ptr != '\t') && (*ptr != '\0'))
				*tok++ = *ptr++;
	}
	*tok++ = '\0';
	return(type);
}

/*
 * fd를 다른 파일로 redirect 시킨다.
 * @param
 * f_name : redirect 시킬 파일 이름
 * file_flags : 파일 open할때의 플래그
 * fd : 파일로 redirect시킬 원래 fd
 * @return success 0 fail -1
 */
int redirect_to_file(char *f_name, int file_flags, int fd) {
    int to_fd;

    if(file_flags & O_CREAT)
        to_fd = open(f_name, file_flags, 0666);
    else
        to_fd = open(f_name, file_flags);

    if(to_fd < 0) {
        fprintf(stderr, "%s : %s\n", f_name, strerror(errno));
        return -1;
    }
    else {
        dup2(to_fd, fd);
    }
    return 0;
}


//TODO : redirect with pipelining, pipelining background process
int child_execute(char **comm, struct re_pi_info *re_pi_info) 
{
    int input_redirection_index = re_pi_info->re_info.in_redi_idx;
    int output_redirection_index = re_pi_info->re_info.out_redi_idx;
    int pipe_idx;

    // set interrupt signal handler to default
    signal(SIGINT, SIG_DFL);
    
    if(input_redirection_index) {
        if(redirect_to_file(comm[input_redirection_index], \
                    O_RDONLY, STDIN_FILENO) < 0) {
            exit(127);
        }
        comm[input_redirection_index] = (char*) '\0';
        re_pi_info->re_info.in_redi_idx = 0;
    }

    if(output_redirection_index) {
        if(redirect_to_file(comm[output_redirection_index], \
                    O_WRONLY|O_CREAT|O_TRUNC, STDOUT_FILENO) < 0) {
            exit(127);
        }
        comm[output_redirection_index] = (char*) '\0';
        re_pi_info->re_info.out_redi_idx = 0;
    }

    execvp(*comm, comm);
    fprintf(stderr, "minish : command not found\n");
    exit(127);

}

int execute(char **comm, int how, struct re_pi_info *re_pi_info)
{
    int	pid;
    //int pstat, wait_ret;
    
    int i=0;
    printf("pipe cnt %d\n ", re_pi_info->pi_info.cnt);
    for(i=0;i<re_pi_info->pi_info.cnt;i++) {
        printf("pipe index %d\n", get_pipe_index(&(re_pi_info->pi_info)));
    }

    if ((pid = fork()) < 0) {
		fprintf(stderr, "minish : fork error\n");
		return(-1);
	}
    else if (pid == 0) {
        child_execute(comm, re_pi_info);
    }
    
	if (how == BACKGROUND) {	// Background execution
        //fprintf(stderr, "%d:%s\n", pid, *comm);
	}
	else {	// Foreground Execution
        /*while((wait_ret = waitpid(pid, &pstat, WNOHANG))==0);
        if(wait_ret < 0) {
            perror("wait err");
        }*/
        pause();
	}
    

	return 0;
}

int newexecute(char **comm, int how, struct comm_list *comm_list)
{
    int i;
    int pid;
    if(comm_list == NULL) return -1;
    
    for(i=0;i<=comm_list->cnt;i++) {
        printf("comm %d %s in %d %s  out %d %s\n", \
                 comm_list->comms[i].comm_idx,
                 comm[comm_list->comms[i].comm_idx],
                 comm_list->comms[i].in_idx,
                 comm[comm_list->comms[i].in_idx],
                 comm_list->comms[i].out_idx,
                 comm[comm_list->comms[i].out_idx]);

        if((pid = fork()) < 0) {
            if(comm_list->comms[i].in_idx != 0) {
                redirect_to_file(comm[comm_list->comms[i].in_idx], O_RDONLY, STDIN_FILENO);
            }
            
            if(comm_list->comms[i].out_idx != 0) {
                redirect_to_file(comm[comm_list->comms[i].out_idx], O_WRONLY | O_CREAT | O_TRUNC, STDOUT_FILENO);
            }
        }
    }

    if (how == BACKGROUND) {
        append_int_node(bg_proc_head, pid);
    }
    else {
        if(waitpid(0, &child_res, 0) < 0) {
            perror("wait error");
        }
    }
}

int parse_and_execute(char *input)
{
	char *arg[1024];
	int	type, how;
	int	quit = FALSE;
	int	narg = 0;
	int	finished = FALSE;
    int re_in_index = 0, re_out_index = 0;
    struct re_pi_info re_pi_info;
    memset(&re_pi_info, 0, sizeof(struct re_pi_info));

    int pipe_idx[1024];
    int pipe_cnt=0;

    int new_comm = TRUE;

    struct comm_list comm_list;
    memset(&comm_list, 0, sizeof(struct comm_list));

	ptr = input;
	tok = tokens;

    
	while (!finished) {
		switch (type = get_token(&arg[narg])) {
		case ARG :
            if(new_comm) {
                set_comm_idx(&comm_list, narg);
                new_comm = FALSE;
            }
			narg++;
			break;
		case EOL :
		case AMPERSAND:
			if (!strcmp(arg[0], "quit")) quit = TRUE;
			else if (!strcmp(arg[0], "exit")) quit = TRUE;
			else if (!strcmp(arg[0], "cd")) {
                if(narg > 1 && strcmp(arg[1], "~")){
                    chdir(arg[1]);
                }
                else {
                    // to home directory
                    chdir(getenv("HOME"));
                }
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
				if (narg != 0) {
                    newexecute(arg, &comm_list);
					//execute(arg, how, &re_pi_info);
                }
			}
			narg = 0;
			if (type == EOL)
				finished = TRUE;
			break; 
        case RE_IN:
            re_pi_info.re_info.in_redi_idx = narg;
            set_in_idx(&comm_list, narg);
            break;
        case RE_OUT:
            re_pi_info.re_info.out_redi_idx = narg;
            set_out_idx(&comm_list, narg);
            break;
        case PIPE:
            add_pipe_index(&(re_pi_info.pi_info), narg);
            set_pipe(&comm_list);
            new_comm = TRUE;
            break;
		}
	}
	return quit;
}

void signal_init() 
{
    signal(SIGINT, sig_handler); 
    signal(SIGCHLD, sigchild_handler);
}

main()
{
	int	quit;
    char cwd[1024];
	printf("msh(%s) # ", getcwd(cwd, 1024));
 
    signal_init();

	while (gets(input)) {
		quit = parse_and_execute(input);
		if (quit) break;
        printf("msh(%s) # ", getcwd(cwd, 1024));
	}
}


