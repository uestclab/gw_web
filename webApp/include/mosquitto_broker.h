#ifndef MOSQUITTO_BROKER_H
#define MOSQUITTO_BROKER_H

#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#include "zlog.h"

#include "broker.h"
#include "gw_control.h"
#include "server.h"
#include "tiny_queue.h"

#define READ  0
#define WRITE 1
/* write file */
typedef struct write_file_t{
	pthread_mutex_t  	mutex;
	int             	enable; // produce_enable: 
	tiny_queue_t*  		queue;
	char           		file_name[1024];
	FILE*          		file;
}write_file_t;

typedef struct queue_item{
	char* buf;
	int   buf_len;
}queue_item;


typedef struct json_set_para{
    char*              system_state_json;
    char*              rssi_open_json;
    char*              rssi_close_json;
}json_set_para;

typedef struct g_broker_para{
    int                system_ready; // device state
	int                enableCallback;
	g_msg_queue_para*  g_msg_queue;
	g_server_para*     g_server;
    g_RegDev_para*     g_RegDev;
	write_file_t*      rssi_file_t;
	int                rssi_state;
    json_set_para      json_set;
	zlog_category_t*   log_handler;
}g_broker_para;


#define	MAX_RSSI_NO	60000
struct rssi_priv{
	struct timeval timestamp;
	int rssi_buf_len;
	char rssi_buf[MAX_RSSI_NO]; 
}__attribute__((packed));

typedef struct ret_byte{
	char* low;
	char* high;
}ret_byte;


int createBroker(char *argv, g_broker_para** g_broker, g_server_para* g_server, g_RegDev_para* g_RegDev, zlog_category_t* handler);
int broker_register_callback_interface(g_broker_para* g_broker);
void process_exception(char* stat_buf, int stat_buf_len, g_broker_para* g_broker);
void destoryBroker(g_broker_para* g_broker);

int inquiry_system_state(g_broker_para* g_broker);
int inquiry_reg_state(g_broker_para* g_broker);
int inquiry_dac_state();

void print_rssi_struct(g_broker_para* g_broker, char* buf, int buf_len);
int control_rssi_state(char *buf, int buf_len, g_broker_para* g_broker); // control by external
int process_rssi_save_file(char* stat_buf, int stat_buf_len, g_broker_para* g_broker);
void clear_rssi_write_status(g_broker_para* g_broker);

#endif//MOSQUITTO_BROKER_H