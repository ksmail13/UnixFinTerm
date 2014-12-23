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

#ifdef DEBUG
#define LOG(format, ...) fprintf(stderr, format, __VA_ARGS__)
#else
#define LOG(format, ...) 
#endif

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
static int pipe_sz;

static struct int_node *bg_proc_head=NULL;

char		*ptr, *tok;

void sig_handler(int signo) 
{
    char cwd[1024];
    switch(signo) {
        case SIGINT:
            printf("\b\b  \b\b");
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
        LOG("%s : %s\n", f_name, strerror(errno));
        return -1;
    }
    else {
        dup2(to_fd, fd);
    }
    return 0;
}

int process_redirection_setting(char **comm, int idx, struct comm_list *comm_list)
{
    if(comm_list->comms[idx].in_idx != 0) {
        redirect_to_file(comm[comm_list->comms[idx].in_idx], O_RDONLY, STDIN_FILENO);
        comm[comm_list->comms[idx].in_idx] = (char*) '\0';
    }

    if(comm_list->comms[idx].out_idx != 0) {
        redirect_to_file(comm[comm_list->comms[idx].out_idx], O_WRONLY | O_CREAT | O_TRUNC, STDOUT_FILENO);
        comm[comm_list->comms[idx].out_idx] = (char*) '\0';
    }
    return 0;
}

int wait_process(struct int_node **list_head, int how) {
    int type = ((how == BACKGROUND)? WNOHANG : 0);
    struct int_node *curr, *temp, *prev=NULL;
    int child_res;
    curr = *list_head;
    prev = curr;

    do {
        if(curr == NULL) break;
        LOG("wait! pid : %d next:%d\n", curr->data, curr->next?curr->next->data:0);
        if(waitpid(curr->data, &child_res, type) > 0) {
            temp = curr;
            curr = prev;
            prev->next = temp->next;
            if(temp == *list_head) *list_head = prev->next;
            free(temp);
        }
        prev = curr;
        curr = curr->next;

    } while(curr != NULL);
}

int pipelining(int pipe_sz, int i)
{
    int j;
    // pipe stdout to pipe input [1] 
    if(pipe_sz > 0) {
        LOG("i : %d pipe_sz: %d\n", i, pipe_sz);
        for(j=0;j<i-1;j++) {
            close(pipes[j][0]);
            close(pipes[j][1]);
        }
        if(pipe_sz > i) {
            close(pipes[i][0]);
            dup2(pipes[i][1], STDOUT_FILENO);
        }
        if(i>0) {
            dup2(pipes[i-1][0], STDIN_FILENO);
            close(pipes[i-1][1]);
        }
        for(j=i+1;j<pipe_sz;j++) {
            close(pipes[j][0]);
            close(pipes[j][1]);
        }
    } 
}


int execute(char **comm, int how, struct comm_list *comm_list)
{
    int i;
    int pid;
    int child_res;
    struct int_node *proc_head=NULL;
    if(comm_list == NULL) return -1;
 
    pipe_sz=0;
    for(i=0;i<comm_list->cnt;i++) {
        pipe(pipes[i]);
        pipe_sz++;
    }

    for(i=0;i<=comm_list->cnt;i++) {
        LOG("comm %d %s in %d %s  out %d %s\n", comm_list->comms[i].comm_idx,comm[comm_list->comms[i].comm_idx], comm_list->comms[i].in_idx, comm[comm_list->comms[i].in_idx],comm_list->comms[i].out_idx,comm[comm_list->comms[i].out_idx]);

        if((pid = fork()) < 0) {
            perror("fork error");
            return 1;
        }
        else if(pid == 0) {
            signal(SIGINT, SIG_DFL);
            pipelining(pipe_sz,i); 
            process_redirection_setting(comm,i, comm_list);

            if(i+1 <= comm_list->cnt) {
                comm[comm_list->comms[i+1].comm_idx] = (char*)NULL;
            }
            execvp(*(comm+comm_list->comms[i].comm_idx), comm+comm_list->comms[i].comm_idx);
            fprintf(stderr, "%s : command not found.\n", *(comm+comm_list->comms[i].comm_idx));
            exit(1);
        }

        LOG("create precess %d\n", pid);
        if(how == BACKGROUND) 
            append_int_node(&bg_proc_head, pid);
        else
            append_int_node(&proc_head, pid);

    }
    
    while(pipe_sz--) {
        LOG("close pipe%d\n", pipe_sz);
        close(pipes[pipe_sz][0]);
        close(pipes[pipe_sz][1]);
    }
    
    if(how == FOREGROUND) {
        wait_process(&proc_head, FOREGROUND);
    }
}

int parse_and_execute(char *input)
{
    char *arg[1024];
    int	type, how;
    int	quit = FALSE;
    int	narg = 0;
    int	finished = FALSE;

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
                        execute(arg, how, &comm_list);
                    }
                }
                narg = 0;
                if (type == EOL)
                    finished = TRUE;
                break; 
            case RE_IN:
                set_in_idx(&comm_list, narg);
                break;
            case RE_OUT:
                set_out_idx(&comm_list, narg);
            break;
        case PIPE:
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
    //signal(SIGQUIT, sig_handler);
    //signal(SIGCHLD, sigchild_handler);
}

main()
{
	int	quit;
    int p_res;
    char cwd[1024];
	printf("msh(%s) # ", getcwd(cwd, 1024));
 
    signal_init();

	while (gets(input)) {
		quit = parse_and_execute(input);
		if (quit) break;

        wait_process(&bg_proc_head, BACKGROUND);

        printf("msh(%s) # ", getcwd(cwd, 1024));
    }
}


