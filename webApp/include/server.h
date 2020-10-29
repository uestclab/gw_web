#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <pthread.h>
#include <sys/eventfd.h>

#include "cJSON.h"
#include "zlog.h"
#include "msg_queue.h"
#include "gw_utility.h"
#include <sys/ioctl.h>
#include <linux/sockios.h>
#include "list.h"
#include "ThreadPool.h"
#include "event_timer.h"

#define TCP_LISTEN_PORT 55055

/**@struct record_action_t
* @brief 记录新接入用户请求操作动作
*/
typedef struct record_action_t{
	int         enable_rssi; ///<  操作rssi*/
	int         enable_rssi_save; ///<  操作存储rssi*/
	int         enable_start_csi; ///<  操作csi*/
	int         enable_csi_save;  ///<  操作存储csi*/
	int         enable_start_constell; ///<  操作星座图*/
}record_action_t;

typedef enum recv_working_state{
    NO_READ_WORK = 0,
    WORKING,
    SOCKET_CLOSE,
    OTHER,
}recv_working_state;

/**@struct g_receive_para
* @brief 网络管理新接入用户网络信息
*/
typedef struct g_receive_para{
	g_msg_queue_para*  g_msg_queue;
	pthread_mutex_t    working_mutex;
	int                working; // check disconnected
	int                connfd;
	int                moreData;
	pthread_mutex_t    send_mutex;
	char*              sendbuf;
	char*              recvbuf;
	zlog_category_t*   log_handler;
}g_receive_para;

/**@struct user_session_node
* @brief 网络管理新接入请求生命周期（页面新打开或刷新---页面关闭）对应状态
*/
typedef struct user_session_node{
	char*                       user_ip;
	unsigned int                node_id;
	struct g_receive_para*      g_receive;
	struct record_action_t*     record_action;
	struct list_head            list;
}user_session_node;

typedef struct epoll_event_node{
	int     			epollfd;
	int 				event_fd;
}epoll_event_node;

typedef struct openwrt_node_t{
	pthread_mutex_t    mutex;
	int                openwrt_link;
	int                openwrt_connfd;
}openwrt_node_t;


/**@struct g_server_para
* @brief 网络管理新接入用户模块依赖资源
*/
typedef struct g_server_para{
	ThreadPool*        g_threadpool;
	g_msg_queue_para*  g_msg_queue;
	event_timer_t* 	   g_timer;
	openwrt_node_t     openwrt_node;
	int                happen_exception;
	int                update_system_time;
	epoll_event_node   epoll_node;
	int                listenfd;
	/* user node session */
	int                user_node_id_init;
	int                user_session_cnt;
	struct list_head   user_session_node_head; // add mutex for user node list
	/* log */
	zlog_category_t*   log_handler;
}g_server_para;


int CreateServerThread(g_server_para** g_server_tmp, 
                       ThreadPool* g_threadpool, g_msg_queue_para* g_msg_queue, event_timer_t* g_timer, zlog_category_t* handler);
int CreateRecvParam(g_receive_para* g_receive, g_msg_queue_para* g_msg_queue, int sock_cli, zlog_category_t* handler);
int unregisterEvent(int fd, g_server_para* g_server);

user_session_node* new_user_node(g_server_para* g_server);
void add_new_user_node_to_list(user_session_node* new_node, g_server_para* g_server);
user_session_node* del_user_node_in_list(int connfd, g_server_para* g_server);
int find_user_node_id(int connfd, g_server_para* g_server);
struct user_session_node* find_user_node_by_user_id(int user_id, g_server_para* g_server);
struct user_session_node* find_user_node_by_connfd(int connfd, g_server_para* g_server);
g_receive_para* findReceiveNode(int connfd, g_server_para* g_server);
void release_receive_resource(g_receive_para* g_receive);

int assemble_frame_and_send(g_receive_para* g_receive, char* buf, int buf_len, int type);
int get_recv_working_state(g_receive_para* g_receive);
void set_recv_working_state(int state, g_receive_para* g_receive);

#endif//SERVER_H
