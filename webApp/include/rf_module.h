#ifndef WEB_RF_MODULE_H
#define WEB_RF_MODULE_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include "cJSON.h"
#include "zlog.h"
#include "msg_queue.h"
#include "mosquitto_broker.h"

typedef struct ret_byte{
	char* low;
	char* high;
}ret_byte;

extern int frequency_rf;
extern int tx_power_state;
extern int rx_gain_state;

/* rf */
int inquiry_rf_info(g_receive_para* tmp_receive, g_broker_para* g_broker);
int process_rf_freq_setting(char* stat_buf, int stat_buf_len, g_broker_para* g_broker);
int open_tx_power();
int close_tx_power();
int rx_gain_normal();
int rx_gain_high();


#endif//WEB_RF_MODULE_H
