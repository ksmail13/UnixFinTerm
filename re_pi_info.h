#ifndef RE_PI_INFO_H

#define RE_PI_INFO_H
struct int_node 
{
    int data;
    struct int_node *next;
}; 

/**
 * 입력받은 커맨드에서 redirection 하는 
 * 파일의 인덱스를 기억하는 구조체
 */
struct redirect_info {
    int in_redi_idx;
    int out_redi_idx;
};

/**
 * 입력받은 커맨드에서 pipe를 통해 
 * 연결할 프로그램들의 인덱스를 기억하는 구조체
 */
struct int_list {
    struct int_node *head;
    int cnt;
};


struct comm_info {
    int comm_idx;
    int in_idx;
    int out_idx;
};

struct comm_list {
    struct comm_info comms[10];
    int cnt;
};

extern struct int_node *create_int_node(int data);

extern int append_int_node(struct int_node **head, int data);
extern int remove_int_node(struct int_node **head, int index);
extern int get_int(struct int_node **head, int index);

extern int add_int(struct int_list *, int index);
//extern int get_int(struct int_list *);


extern int set_comm_idx(struct comm_list *comm_list, int comm_idx);
extern int set_in_idx(struct comm_list *comm_list, int in_idx);
extern int set_out_idx(struct comm_list *comm_list, int out_idx);

extern int set_pipe(struct comm_list *comm_list);
#endif
