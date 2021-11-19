#ifndef THREAD_FUNC_WRAPPER_H
#define THREAD_FUNC_WRAPPER_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include "ThreadPool.h"
#include "cJSON.h"
#include "zlog.h"
#include "server.h"
#include "mosq_broker.h"
#include "rf_module.h"

typedef struct input_args_t{
	int   number_1;
	void* arg_1;
	void* arg_2;
    void* arg_3;
}input_args_t;

/* 30s inquire system display*/
void postTimeOutWorkToThreadPool(g_broker_para* g_broker, ThreadPool* g_threadpool);
/* rf i2c process waste time , release event process main thread */
void postRfWorkToThreadPool(int user_node_id, g_broker_para* g_broker, ThreadPool* g_threadpool);

#endif//THREAD_FUNC_WRAPPER_H
