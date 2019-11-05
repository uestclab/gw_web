#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/syscall.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include "zlog.h"
#include "server.h"
#include "msg_queue.h"
#include "cJSON.h"
#include "web_common.h"
#include "gw_macros_util.h"

#define BUFFER_SIZE 2560*4

void postMsg(long int msg_type, char *buf, int buf_len, g_msg_queue_para* g_msg_queue){
	struct msg_st data;
	data.msg_type = msg_type;
	data.msg_number = msg_type;
	data.msg_len = buf_len;
	if(buf != NULL && buf_len != 0)
		memcpy(data.msg_json,buf,buf_len);
	postMsgQueue(&data,g_msg_queue);
}

void* receive_thread(void* args){
	g_receive_para* g_receive = (g_receive_para*)args;

	g_server_para* g_server = NULL;	
	g_server = container_of(g_receive, g_server_para, g_receive_var);

	zlog_info(g_receive->log_handler,"start receive_thread()\n");
    while(1){
    	//receive(g_receive);
		cJSON* root = cJSON_CreateObject();
		g_server->send_rssi = g_server->send_rssi + 1;
		cJSON_AddNumberToObject(root, "rssi", g_server->send_rssi);
		char* json_buf = cJSON_Print(root);
		cJSON_Delete(root);
		int ret = send(g_receive->connfd,json_buf,strlen(json_buf),0);
		if(ret != strlen(json_buf)){
			zlog_info(g_receive->log_handler," send error :  ret = %d , expect_len = %d \n", ret, strlen(json_buf));
		}
		free(json_buf);
		break;
		char recv_buf[1024];
		ret = recv(g_receive->connfd, recv_buf, 1024,0);
		if(ret > 0){
			zlog_info(g_receive->log_handler, "recv_buf : %s " , recv_buf);
		}
		zlog_info(g_receive->log_handler," recv thread is running \n");
		sleep(1);
    }
	postMsg(MSG_RECEIVE_THREAD_CLOSED,NULL,0,g_receive->g_msg_queue); // pose MSG_RECEIVE_THREAD_CLOSED
    zlog_info(g_receive->log_handler,"end Exit receive_thread()\n");
}

void* runServer(void* args){
	g_server_para* g_server = (g_server_para*)args;

    struct sockaddr_in servaddr; 
    if( (g_server->listenfd = socket(AF_INET,SOCK_STREAM,0)) == -1)
    {
        zlog_error(g_server->log_handler,"create socket error: %s(errno: %d)\n",strerror(errno),errno);
    }
 
    int one = 1;
    setsockopt(g_server->listenfd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));

    memset(&servaddr,0,sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(55055);
 
    if( bind(g_server->listenfd,(struct sockaddr*)&servaddr,sizeof(servaddr)) == -1)
    {
        zlog_error(g_server->log_handler,"bind socket error: %s(errno: %d)\n",strerror(errno),errno);
    }
 
    if( listen(g_server->listenfd,10) == -1)
    {
        zlog_error(g_server->log_handler,"listen socket error: %s(errno: %d)\n",strerror(errno),errno);
    }

    zlog_info(g_server->log_handler,"========waiting for client's request========\n");
	while(1){
		int connfd = -1;
		if( (connfd = accept(g_server->listenfd,(struct sockaddr*)NULL,NULL)) == -1 ){
		    zlog_error(g_server->log_handler,"accept socket error: %s(errno: %d)\n",strerror(errno),errno);
		}else{
			zlog_info(g_server->log_handler," -------------------accept new client , connfd = %d \n", connfd);

			postMsg(MSG_ACCEPT_NEW_USER,(char*)(&connfd),sizeof(int),g_server->g_msg_queue);
		}
		zlog_info(g_server->log_handler,"========waiting for client's request========\n");
	}
}

int CreateServerThread(g_server_para** g_server, g_msg_queue_para* g_msg_queue, zlog_category_t* handler){
	zlog_info(handler,"CreateServerThread()");
	*g_server = (g_server_para*)malloc(sizeof(struct g_server_para));
    (*g_server)->listenfd     = 0;    
	(*g_server)->para_t       = newThreadPara();
	(*g_server)->g_msg_queue  = g_msg_queue;
	(*g_server)->has_user     = 0;
	//(*g_server)->g_receive    = NULL;
	(*g_server)->log_handler  = handler;
	(*g_server)->send_rssi    = 0;
	int ret = pthread_create((*g_server)->para_t->thread_pid, NULL, runServer, (void*)(*g_server));
    if(ret != 0){
        zlog_error(handler,"create CreateServerThread error ! error_code = %d", ret);
		return -1;
    }	
	return 0;
}

int CreateRecvThread(g_receive_para* g_receive, g_msg_queue_para* g_msg_queue, int connfd, zlog_category_t* handler){
	zlog_info(handler,"InitReceThread()");
	//g_receive = (g_receive_para*)malloc(sizeof(struct g_receive_para));
	g_receive->g_msg_queue     = g_msg_queue;
	g_receive->para_t          = newThreadPara();
	g_receive->connfd          = connfd;                     // connfd
	g_receive->log_handler 	   = handler;

	int ret = pthread_create(g_receive->para_t->thread_pid, NULL, receive_thread, (void*)(g_receive));
    if(ret != 0){
        zlog_error(handler,"create CreateRecvThread error ! error_code = %d", ret);
		return -1;
    }	
	return 0;
}
















