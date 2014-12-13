#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <signal.h>

#define COMMAND_SIZE 2048
#define COMMAND_CNT 1024

#define TRUE 1
#define FALSE 0

enum Tokens {EOL='\0', ARG, AMPERSAND='&', RE_IN='<', RE_OUT='>', PIPE='|', EMPTY=' ', EMPTY2='\t'};
enum TokenType {BACKGROUND = 1, RE_IN=2, RE_OUT=4, PIPE=8};

struct commandinfo {
    char *call;
    char *args[1024];
    char *in;
    char *out;
    int arg_cnt;
    int type;
};

int sigchld_handler(int signo) {
    return wait(NULL);
}

static void init_sh() 
{
    signal(SIGCHLD, sigchld_handler);
}

/**
 * print shell header
 */
static inline void print_sh_head() 
{
    static char cwd[1024];

    if(!getcwd(cwd, 1024)) {
        cwd[0] = '\0';
    }

    printf("msh - %s # ", cwd);
}

static int parse_command(char *command, char *command_save[]) 
{
    size_t comm_len = strlen(command);
    int word_start = FALSE;
    
    struct commandinfo com_info;
    memset(&com_info, 0, sizeof(com_info));

    for(;*command;command++) {
        switch(*command) {
            case EMPTY:
            case EMPTY2:
                break;
            case AMPERSAND:
                break;
            case RE_IN:
                break;
            case RE_OUT:
                break;
            default:
                if(!word_start) {
                    word_start = TRUE;
                    if(com_info.call == NULL) {
                        com_info.call = command;
                    }
                    else {
                        com_info.args[com_info.arg_cnt++] = command;
                    }
                }
                break;
        }
    }

}

int main() 
{
    int quit;
    char command[COMMAND_SIZE];
    char *commands[COMMAND_CNT];

    init_sh();

    do {
        fgets(command, COMMAND_SIZE, stdin);
        
    } while(!quit);

    return 0;
}
