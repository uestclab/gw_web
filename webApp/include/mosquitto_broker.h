#ifndef MOSQUITTO_BROKER_H
#define MOSQUITTO_BROKER_H

#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#include "zlog.h"
#include "list.h"

#include "gw_control.h"
#include "gw_utility.h"
#include "server.h"
#include "web_common.h"

typedef struct json_set_para{
    char*              system_state_json;
    char*              rssi_open_json;
    char*              rssi_close_json;
}json_set_para;

typedef struct rssi_module_t{
	int rssi_state;
	int user_cnt;
}rssi_module_t;

typedef struct g_broker_para{
    int                system_ready; // device state
	int64_t            start_time;
	int                enableCallback;
	g_msg_queue_para*  g_msg_queue;
	g_server_para*     g_server;
    g_RegDev_para*     g_RegDev; // gw_control handler
	rssi_module_t      rssi_module;
	struct list_head   rssi_user_node_head;
    json_set_para      json_set;
	zlog_category_t*   log_handler;
}g_broker_para;

typedef struct rssi_user_node{
	g_broker_para*         g_broker;
	int                    connfd;
	write_file_t*          rssi_file_t;
	int                    confirm_delete;
	struct list_head       list;
}rssi_user_node;


#define	MAX_RSSI_NO	60000
struct rssi_priv{
	struct timeval timestamp;
	int rssi_buf_len;
	char rssi_buf[MAX_RSSI_NO]; 
}__attribute__((packed));


int createBroker(char *argv, g_broker_para** g_broker, g_server_para* g_server, g_RegDev_para* g_RegDev, zlog_category_t* handler);
int broker_register_callback_interface(g_broker_para* g_broker);
void process_exception(char* stat_buf, int stat_buf_len, g_broker_para* g_broker);
void destoryBroker(g_broker_para* g_broker);

int inquiry_system_state(g_receive_para* tmp_receive, g_broker_para* g_broker);
int inquiry_reg_state(g_receive_para* tmp_receive, g_broker_para* g_broker);

/* rssi */
void send_rssi_in_event_loop(char* buf, int buf_len, g_broker_para* g_broker);
int open_rssi_state_external(int connfd, g_broker_para* g_broker); // control by external
int close_rssi_state_external(int connfd, g_broker_para* g_broker); // control by external 
int control_rssi_state(char *buf, int buf_len, g_broker_para* g_broker); // control by internal

int process_rssi_save_file(int connfd, char* stat_buf, int stat_buf_len, g_broker_para* g_broker);
void send_rssi_to_save(char* buf, int buf_len, g_broker_para* g_broker);
void inform_stop_rssi_write_thread(int connfd, g_broker_para* g_broker);
void clear_rssi_write_status(rssi_user_node* user_node, g_broker_para* g_broker);


/* test */
void test_process_exception(int state, g_broker_para* g_broker);
 
#endif//MOSQUITTO_BROKER_H
