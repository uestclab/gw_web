#ifndef WEB_SYSTEM_HANDLER_H
#define WEB_SYSTEM_HANDLER_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include "cJSON.h"
#include "zlog.h"
#include "msg_queue.h"
#include "mosquitto_broker.h"

/* eth and link */
int inquiry_statistics(g_receive_para* tmp_receive, g_broker_para* g_broker);

int process_ip_setting(char* stat_buf, int stat_buf_len,zlog_category_t* handler);


#endif//WEB_SYSTEM_HANDLER_H
