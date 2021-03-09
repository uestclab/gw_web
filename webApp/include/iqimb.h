#ifndef _IQIMB_H
#define _IQIMB_H

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>

#include <sys/time.h>
#include <time.h>

#include "zlog.h"
#include "server.h"
#include "msg_queue.h"
#include "cJSON.h"
#include "web_common.h"
#include "gw_macros_util.h"
#include "small_utility.h"
#include "timer.h"
#include "dma_handler.h"

/* iqimb data */
typedef struct iqimb_t{
    g_server_para*      g_server;
	char           		file_name[1024];
	FILE*          		file;
}iqimb_t;

int iqimb_start_cycle(g_server_para* g_server, int cnt_s);
int transfer_data_to_iqimb(char* buf, int buf_len, g_dma_para* g_dma, g_server_para* g_server);




#endif//_IQIMB_H