#ifndef RE_PI_INFO_H

#define RE_PI_INFO_H
struct int_node 
{
    int data;
    struct int_node *next;
}; 

/**
 * 입력받은 커맨드에서 redirection 하는 파일의 인덱스를 기억하는 구조체
 */
struct redirect_info {
    int in_redi_idx;
    int out_redi_idx;
};

/**
 * 입력받은 커맨드에서 pipe를 통해 연결할 프로그램들의 인덱스를 기억하는 구조체
 */
struct pipe_info {
    struct int_node *head;
    int cnt;
};

struct re_pi_info {
    struct redirect_info re_info;
    struct pipe_info pi_info;
    int type;
};


extern struct int_node *create_int_node(int data);

extern int append_int_node(struct int_node **head, int data);
extern int remove_int_node(struct int_node **head, int index);
extern int get_int(struct int_node **head, int index);

extern int add_pipe_index(struct pipe_info *, int index);
extern int get_pipe_index(struct pipe_info *);
#endif