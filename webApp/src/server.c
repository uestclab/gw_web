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
#include "small_utility.h"

#define BUFFER_SIZE 1024 * 4

int processMessage(char* buf, int32_t length, g_receive_para* g_receive){
	int type = myNtohl(buf + 4);
	char* jsonfile = buf + sizeof(int32_t) + sizeof(int32_t);
	if(type == TYPE_SYSTEM_STATE_REQUEST){
		postMsg(MSG_INQUIRY_SYSTEM_STATE, NULL, 0, g_receive, g_receive->g_msg_queue);
	}else if(type == TYPE_REG_STATE_REQUEST){ // json
		postMsg(MSG_INQUIRY_REG_STATE, NULL, 0, g_receive, g_receive->g_msg_queue);
	}else if(type == TYPE_INQUIRY_RSSI_REQUEST){
		postMsg(MSG_INQUIRY_RSSI, NULL, 0, g_receive, g_receive->g_msg_queue);
	}else if(type == TYPE_RSSI_CONTROL){ // save rssi data or not
		postMsg(MSG_CONTROL_RSSI, jsonfile, length-4, g_receive, g_receive->g_msg_queue);
	}else if(type == 22){ // rf_mf_state
		postMsg(MSG_INQUIRY_RF_MF_STATE, jsonfile, length-4, g_receive, g_receive->g_msg_queue);
	} 
	return 0;
}

void receive(g_receive_para* g_receive){
    int size = 0;
    int totalByte = 0;
    int msg_len = 0;
    char* temp_receBuffer = g_receive->recvbuf + 2000; //
    char* pStart = NULL;
    char* pCopy = NULL;

    size = recv(g_receive->connfd, temp_receBuffer, BUFFER_SIZE,0);
    if(size<=0){
		if(size < 0)
			zlog_info(g_receive->log_handler,"recv() size < 0 , size = %d \n" , size);
		if(size == 0)
			zlog_info(g_receive->log_handler,"recv() size = 0\n");
        zlog_info(g_receive->log_handler,"errno = %d ", errno);
        /* check disconnection */
        if(errno != EINTR){
            zlog_info(g_receive->log_handler," need post msg to inform ");
            g_receive->working = 0;
        } 

		return;
    }

    pStart = temp_receBuffer - g_receive->moreData;
    totalByte = size + g_receive->moreData;
    const int MinHeaderLen = sizeof(int32_t);
    while(1){
        if(totalByte <= MinHeaderLen)
        {
            g_receive->moreData = totalByte;
            pCopy = pStart;
            if(g_receive->moreData > 0)
            {
                memcpy(temp_receBuffer - g_receive->moreData, pCopy, g_receive->moreData);
            }
            break;
        }
        if(totalByte > MinHeaderLen)
        {
            msg_len= myNtohl(pStart);
	
            if(totalByte < msg_len + MinHeaderLen )
            {
                g_receive->moreData = totalByte;
                pCopy = pStart;
                if(g_receive->moreData > 0){
                    memcpy(temp_receBuffer - g_receive->moreData, pCopy, g_receive->moreData);
                }
                break;
            } 
            else// at least one message 
            {
				int ret = processMessage(pStart,msg_len,g_receive);
				// move to next message
                pStart = pStart + msg_len + MinHeaderLen;
                totalByte = totalByte - msg_len - MinHeaderLen;
                if(totalByte == 0){
                    g_receive->moreData = 0;
                    break;
                }
            }          
        }
    }	
}

void* receive_thread(void* args){

	pthread_detach(pthread_self());

	g_receive_para* g_receive = (g_receive_para*)args;

	zlog_info(g_receive->log_handler,"start receive_thread()\n");
    while(g_receive->working == 1){ /* if disconnected , exit receive thread */
    	receive(g_receive);
    }

	postMsg(MSG_RECEIVE_THREAD_CLOSED,NULL,0,g_receive,g_receive->g_msg_queue); // pose MSG_RECEIVE_THREAD_CLOSED
    zlog_info(g_receive->log_handler,"end Exit receive_thread()\n");
}

void* runServer(void* args){
	pthread_detach(pthread_self());
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
 
    if( listen(g_server->listenfd,16) == -1) // max number of user in same time
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
            int* buf = (int*)malloc(sizeof(int));
            *buf = connfd;
			postMsg(MSG_ACCEPT_NEW_USER,NULL,0,buf,g_server->g_msg_queue);
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
	(*g_server)->log_handler  = handler;

    INIT_LIST_HEAD(&((*g_server)->user_session_node_head));
	(*g_server)->user_session_cnt    = 0;
	int ret = pthread_create((*g_server)->para_t->thread_pid, NULL, runServer, (void*)(*g_server));
    if(ret != 0){
        zlog_error(handler,"create CreateServerThread error ! error_code = %d", ret);
		return -1;
    }	
	return 0;
}

/*  MSG_ACCEPT_NEW_USER call in event loop */
int CreateRecvThread(g_receive_para* g_receive, g_msg_queue_para* g_msg_queue, int connfd, zlog_category_t* handler){
	zlog_info(handler,"CreateRecvThread()");
	g_receive->g_msg_queue     = g_msg_queue;
	g_receive->para_t          = newThreadPara();
	g_receive->connfd          = connfd;                     // connfd
	g_receive->log_handler 	   = handler;
	g_receive->moreData        = 0;
    g_receive->recvbuf         = (char*)malloc(BUFFER_SIZE);
    g_receive->sendbuf         = (char*)malloc(BUFFER_SIZE);
    pthread_mutex_init(&(g_receive->send_mutex),NULL);
    g_receive->working         = 1;

	int ret = pthread_create(g_receive->para_t->thread_pid, NULL, receive_thread, (void*)(g_receive));
    if(ret != 0){
        zlog_error(handler,"create CreateRecvThread error ! error_code = %d", ret);
		return -1;
    }	
	return 0;
}

/* ------------------------- user session --------------------------------- */
user_session_node* new_user_node(g_server_para* g_server){
    user_session_node* new_node = (user_session_node*)malloc(sizeof(user_session_node));
    new_node->g_receive = (g_receive_para*)malloc(sizeof(g_receive_para));
    new_node->record_action = (record_action_t*)malloc(sizeof(record_action_t));

    new_node->record_action->enable_rssi = 0;
    new_node->record_action->enable_rssi_save = 0;

    list_add_tail(&new_node->list, &g_server->user_session_node_head);
    g_server->user_session_cnt++;

    return new_node;
}

user_session_node* del_user_node_in_list(int connfd, g_server_para* g_server){
    struct list_head *pos, *n;
    struct user_session_node *pnode = NULL;
    list_for_each_safe(pos, n, &g_server->user_session_node_head){
        pnode = list_entry(pos, struct user_session_node, list);
        if(pnode->g_receive->connfd == connfd){
            zlog_info(g_server->log_handler,"find node in del_user_node_in_list() ! connfd = %d", connfd);
            list_del(pos);
            g_server->user_session_cnt--;
            break;
        }
    }
    return pnode;
}

void release_receive_resource(g_receive_para* g_receive){
    close(g_receive->connfd);
    destoryThreadPara(g_receive->para_t);
    free(g_receive->sendbuf);
    free(g_receive->recvbuf);
}


/* ------------------------- send interface-------------------------------- */
/* all call in event loop may be? */
int assemble_frame_and_send(g_receive_para* g_receive, char* buf, int buf_len, int type){
    //zlog_info(g_receive->log_handler," buf : %s",buf);
    int length = buf_len + FRAME_HEAD_ROOM;
    pthread_mutex_lock(&(g_receive->send_mutex));
    char* temp_buf = g_receive->sendbuf;
    *((int32_t*)temp_buf) = htonl(buf_len + sizeof(int32_t));
    *((int32_t*)(temp_buf + sizeof(int32_t))) = htonl(type);
    memcpy(temp_buf + FRAME_HEAD_ROOM,buf,buf_len);
    int ret = send(g_receive->connfd, temp_buf, length, type);
    zlog_info(g_receive->log_handler,"length = %d , type = %d \n", length, type);
    if(ret != length){
        zlog_info(g_receive->log_handler,"ret = %d" , ret);
    }
    pthread_mutex_unlock(&(g_receive->send_mutex));
    return ret;
}








