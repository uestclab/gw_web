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

#define BUFFER_SIZE 1024 * 40

void parseRequestJson(char* request_buf, int request_buf_len, web_msg_t* msg_tmp){
    //{"comment":"comment","type":1,"dst":"mon","op_cmd":0,"localIp":"192.168.0.100","currentTime":"2020-3-9 10:16:22"}
	cJSON * root = NULL;
    cJSON * item = NULL;
    root = cJSON_Parse(request_buf);
    msg_tmp->arg_1 = -1;
    msg_tmp->buf_data = NULL;
    msg_tmp->buf_data_len = 0;
    if(cJSON_HasObjectItem(root,"localIp") == 1){
        item = cJSON_GetObjectItem(root,"localIp");
        memcpy(msg_tmp->localIP, item->valuestring, strlen(item->valuestring)+1);
    }
    if(cJSON_HasObjectItem(root,"currentTime") == 1){
        item = cJSON_GetObjectItem(root,"currentTime");
        memcpy(msg_tmp->currentTime, item->valuestring, strlen(item->valuestring)+1);
        msg_tmp->buf_data_len = strlen(item->valuestring)+1;
    }
    cJSON_Delete(root);
}

int processMessage(char* buf, int32_t length, g_receive_para* g_receive){
	int type = myNtohl(buf + 4);
	char* jsonfile = buf + sizeof(int32_t) + sizeof(int32_t);
    web_msg_t* msg_tmp = (web_msg_t*)malloc(sizeof(web_msg_t));
    msg_tmp->point_addr_1 = g_receive;
    parseRequestJson(jsonfile,length-4,msg_tmp);

    if(type == TYPE_SYSTEM_STATE_REQUEST){
		postMsg(MSG_INQUIRY_SYSTEM_STATE, NULL, 0, msg_tmp, 0, g_receive->g_msg_queue);
	}else if(type == TYPE_REG_STATE_REQUEST){ // json
		postMsg(MSG_INQUIRY_REG_STATE, NULL, 0, msg_tmp, 0, g_receive->g_msg_queue);
	}else if(type == TYPE_INQUIRY_RSSI_REQUEST){
		postMsg(MSG_INQUIRY_RSSI, NULL, 0, msg_tmp, 0, g_receive->g_msg_queue);
	}else if(type == TYPE_RSSI_CONTROL){ // save rssi data or not
		postMsg(MSG_CONTROL_RSSI, jsonfile, length-4, msg_tmp, 0, g_receive->g_msg_queue);
	}else if(type == TYPE_START_CSI){ // start csi 
		postMsg(MSG_START_CSI, NULL, 0, msg_tmp, 0, g_receive->g_msg_queue);
	}else if(type == TYPE_STOP_CSI){ // stop csi
		postMsg(MSG_STOP_CSI, NULL, 0, msg_tmp, 0, g_receive->g_msg_queue);
	}else if(type == TYPE_CONTROL_SAVE_CSI){ // save csi data or not
		postMsg(MSG_CONTROL_SAVE_IQ_DATA, jsonfile, length-4, msg_tmp, 0, g_receive->g_msg_queue);
	}else if(type == TYPE_START_CONSTELLATION){ // start constellation
		postMsg(MSG_START_CONSTELLATION, NULL, 0, msg_tmp, 0, g_receive->g_msg_queue);
	}else if(type == TYPE_STOP_CONSTELLATION){ // stop constellation
		postMsg(MSG_STOP_CONSTELLATION, NULL, 0, msg_tmp, 0, g_receive->g_msg_queue);
	}else if(type == TYPE_OPEN_DISTANCE_APP){ // open distance app
        postMsg(MSG_OPEN_DISTANCE_APP, NULL, 0, msg_tmp, 0, g_receive->g_msg_queue);
    }else if(type == TYPE_CLOSE_DISTANCE_APP){ // close distance app
        postMsg(MSG_CLOSE_DISTANCE_APP, NULL, 0, msg_tmp, 0, g_receive->g_msg_queue);
    }else if(type == TYPE_OPEN_DAC){ // open dac
        postMsg(MSG_OPEN_DAC, NULL, 0, msg_tmp, 0, g_receive->g_msg_queue);
    }else if(type == TYPE_CLOSE_DAC){ // close dac
        postMsg(MSG_CLOSE_DAC, NULL, 0, msg_tmp, 0, g_receive->g_msg_queue);
    }else if(type == TYPE_CLEAR_LOG){ // clear log
        postMsg(MSG_CLEAR_LOG, NULL, 0, msg_tmp, 0, g_receive->g_msg_queue);
    }else if(type == TYPE_RESET){ // reset board
        postMsg(MSG_RESET_SYSTEM, NULL, 0, msg_tmp, 0, g_receive->g_msg_queue);
    }else if(type == TYPE_STATISTICS_INFO){ // request eth and link info
        postMsg(MSG_INQUIRY_STATISTICS, NULL, 0, msg_tmp, 0, g_receive->g_msg_queue);
    }else if(type == TYPE_IP_SETTING){ // set ip
        postMsg(MSG_IP_SETTING, jsonfile, length-4, msg_tmp, 0, g_receive->g_msg_queue);
    }else if(type == TYPE_RF_INFO){ // request RF info
        postMsg(MSG_INQUIRY_RF_INFO, NULL, 0, msg_tmp, 0, g_receive->g_msg_queue);
    }else if(type == TYPE_RF_FREQ_SETTING){
        postMsg(MSG_RF_FREQ_SETTING, jsonfile, length-4, msg_tmp, 0, g_receive->g_msg_queue);
    }else if(type == TYPE_OPEN_TX_POWER){
        postMsg(MSG_OPEN_TX_POWER, NULL, 0, msg_tmp, 0, g_receive->g_msg_queue);
    }else if(type == TYPE_CLOSE_TX_POWER){
        postMsg(MSG_CLOSE_TX_POWER, NULL, 0, msg_tmp, 0, g_receive->g_msg_queue);
    }else if(type == TYPE_OPEN_RX_GAIN){
        postMsg(MSG_OPEN_RX_GAIN, NULL, 0, msg_tmp, 0, g_receive->g_msg_queue);
    }else if(type == TYPE_CLOSE_RX_GAIN){
        postMsg(MSG_CLOSE_RX_GAIN, NULL, 0, msg_tmp, 0, g_receive->g_msg_queue);
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
    pthread_mutex_lock(&(g_receive->working_mutex));
    g_receive->inform_work = 0;
    pthread_mutex_unlock(&(g_receive->working_mutex));
	postMsg(MSG_RECEIVE_THREAD_CLOSED,NULL,0,g_receive,0,g_receive->g_msg_queue); // pose MSG_RECEIVE_THREAD_CLOSED
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
			postMsg(MSG_ACCEPT_NEW_USER,NULL,0,buf,0,g_server->g_msg_queue);
		}
		zlog_info(g_server->log_handler,"========waiting for client's request========\n");
	}
}

/**@brief tcp连接管理网络模块
* @param[in]  g_server              网络连接管理handler
* @param[in]  g_msg_queue           消息队列handler
* @param[in]  handler               zlog
* @return  函数执行结果
* - 0          上报成功
* - 非0        上报失败
*/
int CreateServerThread(g_server_para** g_server, g_msg_queue_para* g_msg_queue, zlog_category_t* handler){
	zlog_info(handler,"CreateServerThread()");
	*g_server = (g_server_para*)malloc(sizeof(struct g_server_para));
    (*g_server)->listenfd     = 0;    
	(*g_server)->para_t       = newThreadPara();
	(*g_server)->g_msg_queue  = g_msg_queue;
	(*g_server)->log_handler  = handler;
    (*g_server)->update_system_time = 0;
    (*g_server)->openwrt_link = 0;
    (*g_server)->openwrt_connfd = 0;

    INIT_LIST_HEAD(&((*g_server)->user_session_node_head));
	(*g_server)->user_session_cnt    = 0;
    (*g_server)->user_node_id_init   = 1;
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
    g_receive->inform_work     = 1;
    pthread_mutex_init(&(g_receive->working_mutex),NULL);
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
    new_node->node_id = g_server->user_node_id_init;
    new_node->g_receive = (g_receive_para*)malloc(sizeof(g_receive_para));
    new_node->record_action = (record_action_t*)malloc(sizeof(record_action_t));

    new_node->record_action->enable_rssi = 0;
    new_node->record_action->enable_rssi_save = 0;

    new_node->record_action->enable_start_csi = 0;
    new_node->record_action->enable_csi_save = 0;

    new_node->record_action->enable_start_constell = 0;

    new_node->user_ip = NULL;

    list_add_tail(&new_node->list, &g_server->user_session_node_head);
    g_server->user_session_cnt++;
    g_server->user_node_id_init++;
    if(g_server->user_node_id_init == 1025){
        g_server->user_node_id_init = 1;
    }
    return new_node;
}

user_session_node* del_user_node_in_list(int connfd, g_server_para* g_server){ // may not find the node ?
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

int find_user_node_id(int connfd, g_server_para* g_server){
    int node_id = -1;
    struct user_session_node *pnode = NULL; 
    list_for_each_entry(pnode, &g_server->user_session_node_head, list) {
        if(pnode->g_receive->connfd == connfd){
            node_id = pnode->node_id;
            break;
        }
    }
    return node_id;
}

struct user_session_node* find_user_node_by_connfd(int connfd, g_server_para* g_server){
    struct user_session_node *pnode = NULL;
    struct user_session_node *tmp_pnode = NULL;  
    list_for_each_entry(pnode, &g_server->user_session_node_head, list) {
        if(pnode->g_receive->connfd == connfd){
            tmp_pnode = pnode;
            break;
        }
    }
    return tmp_pnode;
}

struct user_session_node* find_user_node_by_user_id(int user_id, g_server_para* g_server){
    struct user_session_node *pnode = NULL;
    struct user_session_node *tmp_pnode = NULL;  
    list_for_each_entry(pnode, &g_server->user_session_node_head, list) {
        if(pnode->node_id == user_id){
            tmp_pnode = pnode;
            break;
        }
    }
    return tmp_pnode;
}

g_receive_para* findReceiveNode(int connfd, g_server_para* g_server){
	struct user_session_node *pnode = NULL;
	g_receive_para* tmp_receive = NULL;
	pnode = find_user_node_by_connfd(connfd, g_server);
	if(pnode != NULL){
		tmp_receive = pnode->g_receive;
	}
	return tmp_receive;
}

void release_receive_resource(g_receive_para* g_receive){
    pthread_mutex_destroy(&(g_receive->send_mutex));
    pthread_mutex_destroy(&(g_receive->working_mutex));
    close(g_receive->connfd);
    destoryThreadPara(g_receive->para_t);
    free(g_receive->sendbuf);
    free(g_receive->recvbuf);
    free(g_receive);
    g_receive = NULL;
}

int checkReceiveWorkingState(g_receive_para* g_receive){
    pthread_mutex_lock(&(g_receive->working_mutex));
    int ret = g_receive->inform_work;
    pthread_mutex_unlock(&(g_receive->working_mutex));
    return ret;
}

/* ------------------------- send interface-------------------------------- */
/* all call in event loop may be? */

/**@brief     后端发送消息接口
* @param[in]  g_receive              对应不同的连接接收handler(前端发送服务请求到后端，该请求对应不同的用户，后端需根据该handler正确回复请求的发起者)
* @param[in]  buf                    消息buffer
* @param[in]  buf_len                消息buffer长度（字符串不包括字符串结束符）
* @param[in]  type                   回复前端消息类型
* @return  函数执行结果
* - 0          上报成功
* - 非0        上报失败
*/
int assemble_frame_and_send(g_receive_para* g_receive, char* buf, int buf_len, int type){
    //zlog_info(g_receive->log_handler," buf : %s",buf);
    if(g_receive == NULL){
        return -99;
    }
    if(checkReceiveWorkingState(g_receive) == 0){
        zlog_info(g_receive->log_handler, "Receive Working State is zero !");
        return -98;
    }
    int length = buf_len + FRAME_HEAD_ROOM;
    pthread_mutex_lock(&(g_receive->send_mutex));
    char* temp_buf = g_receive->sendbuf;
    *((int32_t*)temp_buf) = htonl(buf_len + sizeof(int32_t));
    *((int32_t*)(temp_buf + sizeof(int32_t))) = htonl(type);
    memcpy(temp_buf + FRAME_HEAD_ROOM,buf,buf_len);
    int ret = send(g_receive->connfd, temp_buf, length, type);
    if(ret != length){
        zlog_info(g_receive->log_handler,"ret = %d" , ret);
    }

    if(TYPE_SYSTEM_STATE_RESPONSE == type || TYPE_SYSTEM_STATE_EXCEPTION == type){
        //zlog_info(g_receive->log_handler, "system state json : %s ", buf);
    }

    pthread_mutex_unlock(&(g_receive->send_mutex));
    return ret;
}








