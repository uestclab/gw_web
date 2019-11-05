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

void c_compiler_builtin_macro(zlog_category_t* zlog_handler)
{
	zlog_info(zlog_handler,"gcc compiler ver:%s\n",__VERSION__);
	zlog_info(zlog_handler,"this version built time is:[%s  %s]\n",__DATE__,__TIME__);
	//printf("gcc compiler ver:%s\n",__VERSION__);
	//printf("this version built time is:[%s  %s]\n",__DATE__,__TIME__);
}

int main(int argc,char** argv)
{
	zlog_category_t *zlog_handler = serverLog("../conf/zlog_default.conf");
	//zlog_category_t *zlog_handler = serverLog("./zlog_default.conf");

	zlog_info(zlog_handler,"start webAdapter process\n");

	c_compiler_builtin_macro(zlog_handler);
	
	/* msg_queue */
	const char* pro_path = "/tmp/handover_test/";
	int proj_id = 'w';
	g_msg_queue_para* g_msg_queue = createMsgQueue(pro_path, proj_id, zlog_handler);
	if(g_msg_queue == NULL){
		zlog_info(zlog_handler,"No msg_queue created \n");
		return 0;
	}
	zlog_info(zlog_handler, "g_msg_queue->msgid = %d \n", g_msg_queue->msgid);

	/* server thread */
	g_server_para* g_server = NULL;
	int state = CreateServerThread(&g_server, g_msg_queue, zlog_handler);
	if(state == -1 || g_server == NULL){
		zlog_info(zlog_handler,"No server thread created \n");
		return 0;
	}

	eventLoop(g_server, g_msg_queue, zlog_handler);

	closeServerLog();
    return 0;
}

