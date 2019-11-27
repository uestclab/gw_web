#ifndef EVENT_PROCESS_H
#define EVENT_PROCESS_H
#include <stdio.h>  
#include <pthread.h>  
#include <stdlib.h>  
#include <unistd.h>  
#include <signal.h>   
#include <string.h>  
#include <errno.h>
#include "zlog.h"

#include "msg_queue.h"
#include "ThreadPool.h"
#include "server.h"
#include "mosquitto_broker.h"



void eventLoop(g_server_para* g_server, g_broker_para* g_broker, g_msg_queue_para* g_msg_queue, ThreadPool* g_threadpool, zlog_category_t* zlog_handler); 

void del_user(int connfd, g_server_para* g_server, g_broker_para* g_broker, ThreadPool* g_threadpool);
void record_rssi_enable(int connfd, g_server_para* g_server);
int record_rssi_save_enable(int connfd, char* stat_buf, int stat_buf_len, g_server_para* g_server);

#endif//EVENT_PROCESS_H


