#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include "zlog.h"

#include "msg_queue.h"
#include "event_process.h"
#include "mosquitto_broker.h"
#include "gw_control.h"
#include "web_common.h"


zlog_category_t * serverLog(const char* path){
	int rc;
	zlog_category_t *zlog_handler = NULL;

	rc = zlog_init(path);

	if (rc) {
		printf("init serverLog failed\n");
		return NULL;
	}

	zlog_handler = zlog_get_category("webAdapterlog");

	if (!zlog_handler) {
		printf("get cat fail\n");

		zlog_fini();
		return NULL;
	}

	return zlog_handler;
}

void closeServerLog(){
	zlog_fini();
}

int main(int argc,char** argv)
{
	zlog_category_t *zlog_handler = serverLog("../conf/zlog_default.conf");
	//zlog_category_t *zlog_handler = serverLog("./zlog_default.conf");

	zlog_info(zlog_handler,"start webAdapter process\n");

	zlog_info(zlog_handler,"this version built time is:[%s  %s]\n",__DATE__,__TIME__);
	
	/* msg_queue */
	const char* pro_path = "/tmp/handover_test/";
	int proj_id = 'w';
	g_msg_queue_para* g_msg_queue = createMsgQueue(pro_path, proj_id, zlog_handler);
	if(g_msg_queue == NULL){
		zlog_info(zlog_handler,"No msg_queue created \n");
		return 0;
	}
	zlog_info(zlog_handler, "g_msg_queue->msgid = %d \n", g_msg_queue->msgid);

	/* reg dev */
	g_RegDev_para* g_RegDev = NULL;
	int state = initRegdev(&g_RegDev, zlog_handler);
	if(state != 0 ){
		zlog_info(zlog_handler,"initRegdev create failed !");
		return 0;
	}

	/* server thread */
	g_server_para* g_server = NULL;
	state = CreateServerThread(&g_server, g_msg_queue, zlog_handler);
	if(state == -1 || g_server == NULL){
		zlog_info(zlog_handler,"No server thread created \n");
		return 0;
	}

	/* broker handler */
	g_broker_para* g_broker = NULL;
	state = createBroker(argv[0], &g_broker, g_server, g_RegDev, zlog_handler);
	if(state != 0 || g_server == NULL){
		zlog_info(zlog_handler,"No Broker created \n");
		return 0;
	}

	struct msg_st data;
	data.msg_type = MSG_INQUIRY_SYSTEM_STATE;
	data.msg_number = MSG_INQUIRY_SYSTEM_STATE;
	data.msg_len = 0;
	postMsgQueue(&data,g_msg_queue);
	eventLoop(g_server, g_broker, g_msg_queue, zlog_handler);

	closeServerLog();
    return 0;
}

