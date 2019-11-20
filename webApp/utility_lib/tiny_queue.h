#ifndef __TINY_QUEUE__
#define __TINY_QUEUE__

#include <pthread.h>

typedef struct tiny_queue_t{
    struct tiny_linked_list_t *head;
    struct tiny_linked_list_t *tail;
    pthread_mutex_t mutex;
    pthread_cond_t  wakeup;
}tiny_queue_t;

typedef struct tiny_linked_list_t{
    void *data;
    struct tiny_linked_list_t *next;
}tiny_linked_list_t;


tiny_queue_t* tiny_queue_create();

int tiny_queue_push(tiny_queue_t *queue, void *data);

void *tiny_queue_pop(tiny_queue_t *queue);

int tiny_queue_destory(tiny_queue_t *queue);


#endif //__TINY_QUEUE__