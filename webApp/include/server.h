#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <pthread.h>
#include "cJSON.h"
#include "zlog.h"
#include "msg_queue.h"
#include "gw_utility.h"
#include <sys/ioctl.h>
#include <linux/sockios.h>

typedef struct g_receive_para{
	g_msg_queue_para*  g_msg_queue;
	para_thread*       para_t;
	int                connfd;
	int                moreData;
	char*              sendbuf;
	char*              recvbuf;
	zlog_category_t*   log_handler;
}g_receive_para;

typedef struct g_server_para{
	g_msg_queue_para*  g_msg_queue;
	g_receive_para     g_receive_var;
	int                listenfd;
	int                has_user; // only one user in working
	para_thread*       para_t;
	zlog_category_t*   log_handler;
	// debug test
	int                send_rssi;
}g_server_para;


int CreateServerThread(g_server_para** g_server, g_msg_queue_para* g_msg_queue, zlog_category_t* handler);
int CreateRecvThread(g_receive_para* g_receive, g_msg_queue_para* g_msg_queue, int sock_cli, zlog_category_t* handler);


#endif//SERVER_H
