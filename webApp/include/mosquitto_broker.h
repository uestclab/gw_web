#ifndef MOSQUITTO_BROKER_H
#define MOSQUITTO_BROKER_H

#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#include "zlog.h"

#include "broker.h"
#include "gw_control.h"
#include "server.h"

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


int control_rssi_state(char *buf, int buf_len, g_broker_para* g_broker); // control by external
void close_rssi(g_broker_para* g_broker); // control by internal

#endif//MOSQUITTO_BROKER_H