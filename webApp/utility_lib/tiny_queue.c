#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>

#include "tiny_queue.h"


tiny_queue_t* tiny_queue_create(){
    struct tiny_queue_t* queue = (tiny_queue_t*)malloc(sizeof(tiny_queue_t));
    if(queue == NULL){
        return NULL;
    }

    queue->head = NULL;
    queue->tail = NULL;

    if(pthread_mutex_init(&queue->mutex,NULL) == 0 &&
       pthread_cond_init(&queue->wakeup,NULL) == 0){
           return queue;
    }

    free(queue);
    return NULL;
}

int tiny_queue_push(tiny_queue_t *queue, void *data){
    tiny_linked_list_t* new_node = (tiny_linked_list_t*)malloc(sizeof(tiny_linked_list_t));
    
    if(new_node == NULL){
        return -1;
    }

    new_node->data = data;
    new_node->next = NULL;

    pthread_mutex_lock(&queue->mutex);
    if(queue->head == NULL && queue->tail == NULL){
        queue->head = queue->tail = new_node;
    }else{
        queue->tail->next = new_node;
        queue->tail = new_node;
    }

    pthread_mutex_unlock(&queue->mutex);
    pthread_cond_signal(&queue->wakeup);

    return 0;
}

void *tiny_queue_pop(tiny_queue_t *queue){
    pthread_mutex_lock(&queue->mutex);

    while(queue->head == NULL){
        pthread_cond_wait(&queue->wakeup,&queue->mutex);
    }

    tiny_linked_list_t* current_head = queue->head;
    void* data = current_head->data;

    if(queue->head == queue->tail){
        queue->head = queue->tail = NULL;
    }else{
        queue->head = queue->head->next;
    }

    free(current_head);
    pthread_mutex_unlock(&queue->mutex);
    
    return data;
}

int tiny_queue_destory(tiny_queue_t *queue){
    if(pthread_mutex_destroy(&queue->mutex) != 0 || pthread_cond_destroy(&queue->wakeup) != 0){
        return -1;
    }

    tiny_linked_list_t *node, *next;
    node = queue->head;

    if(node != NULL){
        while(node->next != NULL){
            next = node->next;
            free(node);
            node = next;
        }
    }

    free(queue);

    return 0;

}

#ifdef TEST
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>

#include "tiny_queue.h"

#define NUMTHREADS 10

// The queue accepts pointers so we can pass arbitrary structs into our queue.
typedef struct work_t {
  int a;
  int b;
} work_t;

// This function will be passed in to `pthread_create`
void *do_work(void *arg) {
  // Store queue argument as new variable
  tiny_queue_t *my_queue = arg;

  // Pop work off of queue, thread blocks here till queue has work
  work_t *work = tiny_queue_pop(my_queue);

  // Do work, we're not really doing anything here, but you
  // could
  printf("(%i * %i) = %i\n", work->a, work->b, work->a * work->b);
  free(work);
  return 0;
}

int main(){
  pthread_t threadpool[NUMTHREADS];
  int i;

  // Create new queue
  tiny_queue_t *my_queue = tiny_queue_create();

  if (my_queue == NULL) {
    fprintf(stderr, "Cannot creare the queue\n");
    return -1;
  }

  // Create thread pool
  for (i=0; i < NUMTHREADS; i++) {
    // Pass queue to new thread
    if (pthread_create(&threadpool[i], NULL, do_work, my_queue) != 0) {
      fprintf(stderr, "Unable to create worker thread\n");
      return -1;
    }
  }

  // Produce "work" and add it on to the queue
  for (i=0; i < NUMTHREADS; i++) {
    // Allocate an object to push onto queue
    struct work_t* work = (struct work_t*)malloc(sizeof(struct work_t));
    work->a = i+1;
    work->b = 2 *(i+1);

    // Every time an item is added to the queue, a thread that is
    // Blocked by `tiny_queue_pop` will unblock
    if (tiny_queue_push(my_queue, work) != 0) {
        fprintf(stderr, "Cannot push an element in the queue\n");
        return -1;
    }
  }

  // sleep(4);


  // Join all the threads
  for (i=0; i < NUMTHREADS; i++) {
    pthread_join(threadpool[i], NULL); // wait for producer to exit
  }

  if (tiny_queue_destory(my_queue) != 0) {
    fprintf(stderr, "Cannot destroy the queue, but it doesn't matter becaus");
    fprintf(stderr, "e the program will exit instantly\n");
    return -1;
  } else {
    return 0;
  }
}
#endif