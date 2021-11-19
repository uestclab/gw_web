#ifndef MOSQ_BROKER_H
#define MOSQ_BROKER_H

#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#include "zlog.h"
#include "list.h"

#include "gw_register.h"
#include "gw_utility.h"
#include "server.h"
#include "web_common.h"
#include "mosquitto_client.h"

/**@struct json_set_para
* @brief 程序自身读取的json文件对应指针
*/
typedef struct json_set_para{
    char*              system_state_json;
    char*              rssi_open_json;
    char*              rssi_close_json;
}json_set_para;

/**@struct rssi_module_t
* @brief 定义rssi模块信息
*/
typedef struct rssi_module_t{
	int                rssi_state;
	int                user_cnt; 
}rssi_module_t;

/**@struct g_broker_para
* @brief 定义内部消息队列代理模块依赖资源
*/
typedef struct g_broker_para{
    int                system_ready; // device state
	int64_t            start_time;
	int64_t            update_acc_time;
	g_msg_queue_para*  g_msg_queue;
	g_server_para*     g_server;
    g_RegDev_para*     g_RegDev; // gw_register handler
	rssi_module_t*     rssi_module;
	struct list_head   rssi_user_node_head;
	zlog_category_t*   log_handler;
}g_broker_para;

/**@struct rssi_user_node
* @brief 定义使用rssi用户节点
*/
typedef struct rssi_user_node{
	g_broker_para*         g_broker;
	int                    connfd;
	write_file_t*          rssi_file_t;
	int                    confirm_delete;
	struct list_head       list;
}rssi_user_node;

int createBroker(char *argv, g_broker_para** g_broker, g_server_para* g_server, g_RegDev_para* g_RegDev, zlog_category_t* handler);
void process_system_state_topic(char* topic_buf, int topic_buf_len, g_broker_para* g_broker);
void destoryBroker(g_broker_para* g_broker);

int inquiry_system_state(g_receive_para* tmp_receive, g_broker_para* g_broker);
int inquiry_reg_state(g_receive_para* tmp_receive, g_broker_para* g_broker);

/* rssi */
void send_rssi_in_event_loop(char* buf, int buf_len, g_broker_para* g_broker);
int open_rssi_state_external(int connfd, g_broker_para* g_broker); // control by external
int close_rssi_state_external(int connfd, g_broker_para* g_broker); // control by external 
int control_rssi_state(int cmd, g_broker_para* g_broker); // control by internal

int process_rssi_save_file(int connfd, char* stat_buf, int stat_buf_len, g_broker_para* g_broker);
void send_rssi_to_save(char* buf, int buf_len, g_broker_para* g_broker);
void inform_stop_rssi_write_thread(int connfd, g_broker_para* g_broker);
void clear_rssi_write_status(rssi_user_node* user_node, g_broker_para* g_broker);

void tmp_system_state(g_receive_para* tmp_receive, g_broker_para* g_broker);


/* test */
 
#endif//MOSQ_BROKER_H
