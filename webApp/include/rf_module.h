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

/* new 20200225 */
extern double local_oscillator_lock_state;
extern double rf_temper;
extern double rf_current;

extern double bb_current;
extern double device_temper;
extern double bb_vs;
extern double adc_temper;
extern double zynq_temper;

/* rf */
char* inquiry_rf_info(g_receive_para* tmp_receive, g_broker_para* g_broker);
int process_rf_freq_setting(char* stat_buf, int stat_buf_len, g_broker_para* g_broker);
int open_tx_power();
int close_tx_power();
int rx_gain_normal();
int rx_gain_high();

double get_local_oscillator_lock_state(zlog_category_t* handler);
double get_rf_temper();
double get_rf_current(zlog_category_t* handler);
double get_bb_current();
double get_device_temper(zlog_category_t* handler);
double get_bb_vs();
double get_adc_temper();
double get_zynq_temper(zlog_category_t* handler);


#endif//WEB_RF_MODULE_H
