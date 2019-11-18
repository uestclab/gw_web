#ifndef LIST_QUEUE_H
#define LIST_QUEUE_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <pthread.h>
#include "cJSON.h"
#include "zlog.h"
#include "adlist.h"


typedef struct g_list_queue{
	list*  			   g_list;
	int                num_item;
	zlog_category_t*   log_handler;
}g_list_queue;


g_list_queue* CreateListQueue(zlog_category_t* handler);


#endif//LIST_QUEUE_H