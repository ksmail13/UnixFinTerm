#include "re_pi_info.h"
#include <stdlib.h>

struct int_node *create_int_node(int data)
{
    struct int_node *new_node;
    if((new_node = calloc(1, sizeof(struct int_node))) == NULL) {
        perror("memory alloc error");
        exit(1);
    }
    new_node->data = data;
    return new_node;
}

int append_int_node(struct int_node **head, int data) 
{
    struct int_node *curr;
    if(head == NULL) return 0;

    if(*head == NULL) {
        *head = create_int_node(data);
        return 1;
    }
    curr = *head;
    while(curr->next) {
        curr = curr->next;
    }

    curr->next = create_int_node(data-curr->data);
    return 1;
}

int remove_int_node(struct int_node **head, int index) 
{
    struct int_node *curr, *temp;
    int cnt;
    
    if(head == NULL) return 0;
    if(*head == NULL) {
        return 0;
    }

    curr = *head;

    for(cnt=0; curr->next && cnt < index-1;) {
        curr = curr->next;
        cnt++;
    }
    temp = curr->next;
    if(temp != NULL)
        curr->next = temp->next;
    
    free(temp);
    return 1;
}

int get_int(struct int_node **head, int index) 
{
    struct int_node *curr;
    int cnt;
    if(head == NULL) return 0xFFFFFFFF;
    if(*head == NULL) {
        return 0xFFFFFFFF;
    }

    curr = *head;

    for(cnt=0; curr->next && cnt < index;) {
        curr = curr->next;
        cnt++;
    }

    return curr->data;
}


int add_pipe_index(struct pipe_info *p_info, int index) 
{
    append_int_node(&(p_info->head), index);
    p_info->cnt++;
}

int get_pipe_index(struct pipe_info *p_info) 
{
    int ret;
    ret = get_int(&(p_info->head), 0);
    remove_int_node(&(p_info->head), 0);
    p_info->cnt--;
    return ret;
}

int set_comm_idx(struct comm_list *comm_list, int comm_idx)
{
    if(comm_list == NULL) return -1;

    comm_list->comms[comm_list->cnt].comm_idx = comm_idx;
    return 0;
}

int set_in_idx(struct comm_list *comm_list, int in_idx)
{
    if(comm_list == NULL) return -1;

    comm_list->comms[comm_list->cnt].in_idx = in_idx;
    return 0;
}

int set_out_idx(struct comm_list *comm_list, int out_idx)
{
    if(comm_list == NULL) return -1;

    comm_list->comms[comm_list->cnt].out_idx = out_idx;
    return 0;
}

int set_pipe(struct comm_list *comm_list)
{
    if (comm_list == NULL) return -1;

    comm_list->cnt++;
}
