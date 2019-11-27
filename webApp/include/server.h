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
#include "list.h"

typedef struct record_action_t{
	int         enable_rssi;
	int         enable_rssi_save;
}record_action_t;

typedef struct g_receive_para{
	g_msg_queue_para*  g_msg_queue;
	para_thread*       para_t;
	int                working; // check disconnected
	int                connfd;
	int                moreData;
	pthread_mutex_t    send_mutex;
	char*              sendbuf;
	char*              recvbuf;
	zlog_category_t*   log_handler;
}g_receive_para;

typedef struct user_session_node{
	struct g_receive_para*      g_receive;
	struct record_action_t*     record_action;
	struct list_head            list;
}user_session_node;

typedef struct g_server_para{
	g_msg_queue_para*  g_msg_queue;
	int                listenfd;
	int                user_session_cnt;
	struct list_head   user_session_node_head;
	para_thread*       para_t;
	zlog_category_t*   log_handler;
}g_server_para;


int CreateServerThread(g_server_para** g_server, g_msg_queue_para* g_msg_queue, zlog_category_t* handler);
int CreateRecvThread(g_receive_para* g_receive, g_msg_queue_para* g_msg_queue, int sock_cli, zlog_category_t* handler);

user_session_node* new_user_node(g_server_para* g_server);
user_session_node* del_user_node_in_list(int connfd, g_server_para* g_server);
void release_receive_resource(g_receive_para* g_receive);

int assemble_frame_and_send(g_receive_para* g_receive, char* buf, int buf_len, int type);


#endif//SERVER_H
