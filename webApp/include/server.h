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

/**@struct g_receive_para
* @brief 网络管理新接入用户网络信息
*/
typedef struct g_receive_para{
	g_msg_queue_para*  g_msg_queue;
	para_thread*       para_t;
	pthread_mutex_t    working_mutex;
	int                working; // check disconnected
	int                inform_work;
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

/**@struct g_server_para
* @brief 网络管理新接入用户模块依赖资源
*/
typedef struct g_server_para{
	g_msg_queue_para*  g_msg_queue;
	int                update_system_time;
	int                listenfd;
	int                user_node_id_init;
	int                user_session_cnt;
	struct list_head   user_session_node_head;
	para_thread*       para_t;
	zlog_category_t*   log_handler;
}g_server_para;


int CreateServerThread(g_server_para** g_server, g_msg_queue_para* g_msg_queue, zlog_category_t* handler);
int CreateRecvThread(g_receive_para* g_receive, g_msg_queue_para* g_msg_queue, int sock_cli, zlog_category_t* handler);

user_session_node* new_user_node(g_server_para* g_server);
user_session_node* del_user_node_in_list(int connfd, g_server_para* g_server);
int find_user_node_id(int connfd, g_server_para* g_server);
struct user_session_node* find_user_node_by_user_id(int user_id, g_server_para* g_server);
struct user_session_node* find_user_node_by_connfd(int connfd, g_server_para* g_server);
g_receive_para* findReceiveNode(int connfd, g_server_para* g_server);
void release_receive_resource(g_receive_para* g_receive);

int assemble_frame_and_send(g_receive_para* g_receive, char* buf, int buf_len, int type);


#endif//SERVER_H
