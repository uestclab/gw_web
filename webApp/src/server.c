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
#include <sys/epoll.h>

#include "zlog.h"
#include "server.h"
#include "msg_queue.h"
#include "cJSON.h"
#include "web_common.h"
#include "gw_macros_util.h"
#include "small_utility.h"
#include "utility.h"
#include "timer.h"
#include "openwrt.h"

// 表驱动
typedef struct frame_idx_msg 
{ 
    int frame_type; 
    int msg_type; 
}frame_idx_msg_st;
frame_idx_msg_st idx_msg[] = 
{ 
        {TYPE_SYSTEM_STATE_REQUEST, MSG_INQUIRY_SYSTEM_STATE}, 
        {TYPE_REG_STATE_REQUEST, MSG_INQUIRY_REG_STATE}, 
        {TYPE_INQUIRY_RSSI_REQUEST, MSG_INQUIRY_RSSI},
        {TYPE_START_CSI, MSG_START_CSI},
        {TYPE_STOP_CSI, MSG_STOP_CSI},
        {TYPE_START_CONSTELLATION, MSG_START_CONSTELLATION},
        {TYPE_STOP_CONSTELLATION, MSG_STOP_CONSTELLATION},
        {TYPE_OPEN_DISTANCE_APP, MSG_OPEN_DISTANCE_APP},
        {TYPE_CLOSE_DISTANCE_APP, MSG_CLOSE_DISTANCE_APP},
        {TYPE_OPEN_DAC, MSG_OPEN_DAC},
        {TYPE_CLOSE_DAC, MSG_CLOSE_DAC},
        {TYPE_CLEAR_LOG, MSG_CLEAR_LOG},
        {TYPE_RESET, MSG_RESET_SYSTEM}, 
        {TYPE_STATISTICS_INFO, MSG_INQUIRY_STATISTICS},
        {TYPE_RF_INFO, MSG_INQUIRY_RF_INFO},
        {TYPE_OPEN_TX_POWER, MSG_OPEN_TX_POWER},
        {TYPE_CLOSE_TX_POWER, MSG_CLOSE_TX_POWER},
        {TYPE_OPEN_RX_GAIN, MSG_OPEN_RX_GAIN},
        {TYPE_CLOSE_RX_GAIN, MSG_CLOSE_RX_GAIN},
        {TYPE_RSSI_CONTROL, MSG_CONTROL_RSSI}, 
        {TYPE_CONTROL_SAVE_CSI, MSG_CONTROL_SAVE_IQ_DATA}, 
        {TYPE_IP_SETTING, MSG_IP_SETTING}, 
        {TYPE_RF_FREQ_SETTING, MSG_RF_FREQ_SETTING}, 
        {TYPE_OPENWRT_KEEPALIVE, TYPE_OPENWRT_KEEPALIVE_RESPONSE}, // fast response and check by timer
};

void process_no_json_fun(int frame_type, char *buf, int buf_len, void* tmp_data, int tmp_data_len, g_receive_para* g_receive, void* pri_ptr){
    // add frame_type and frame type index search
    int type_num = sizeof(idx_msg) / sizeof(frame_idx_msg_st); 
    for (int i = 0; i < type_num; i++) 
    { 
        if (idx_msg[i].frame_type == frame_type) 
        { 
            postMsg(idx_msg[i].msg_type, NULL, 0, tmp_data, 0, g_receive->g_msg_queue);
            return; 
        } 
    }
}

void process_json_fun(int frame_type, char *buf, int buf_len, void* tmp_data, int tmp_data_len, g_receive_para* g_receive, void* pri_ptr){
    int type_num = sizeof(idx_msg) / sizeof(frame_idx_msg_st); 
    for (int i = 0; i < type_num; i++) 
    { 
        if (idx_msg[i].frame_type == frame_type) 
        { 
            postMsg(idx_msg[i].msg_type, buf, buf_len, tmp_data, 0, g_receive->g_msg_queue);
            return; 
        } 
    }
}

/* openwrt keepAlive check */
void fast_keepalive_response_fun(int frame_type, char *buf, int buf_len, void* tmp_data, int tmp_data_len, g_receive_para* g_receive, void* pri_ptr){
    int type_num = sizeof(idx_msg) / sizeof(frame_idx_msg_st); 
    for (int i = 0; i < type_num; i++) 
    { 
        if (idx_msg[i].frame_type == frame_type) 
        { 
            assemble_frame_and_send(g_receive, NULL, 0, idx_msg[i].msg_type); // 无风险，对方发送过快，收到消息也在accept之后，g_receive已经准备就绪

            return; 
        } 
    }
}

void process_keepalive_response_fun(int frame_type, char *buf, int buf_len, void* tmp_data, int tmp_data_len, g_receive_para* g_receive, void* pri_ptr){
    int type_num = sizeof(idx_msg) / sizeof(frame_idx_msg_st); 
    if (TYPE_OPENWRT_KEEPALIVE_RESPONSE == frame_type) 
    { 
        int connfd =  g_receive->connfd; // 待修改传入方式
        g_server_para* g_server = (g_server_para*)pri_ptr;
        setOpenwrtState(g_server);

        return; 
    } 
}

typedef void (*PROC_MSG_FUN)(int frame_type, char *buf, int buf_len, void* tmp_data, int tmp_data_len, g_receive_para* g_receive, void* pri_ptr);
typedef struct __msg_fun_st 
{ 
    const int frame_type;//消息类型 
    PROC_MSG_FUN fun_ptr;//函数指针 
}msg_fun_st;
msg_fun_st msg_flow[] = 
{ 
        {TYPE_SYSTEM_STATE_REQUEST, process_no_json_fun}, 
        {TYPE_REG_STATE_REQUEST, process_no_json_fun}, 
        {TYPE_INQUIRY_RSSI_REQUEST, process_no_json_fun},
        {TYPE_START_CSI, process_no_json_fun},
        {TYPE_STOP_CSI, process_no_json_fun},
        {TYPE_START_CONSTELLATION, process_no_json_fun},
        {TYPE_STOP_CONSTELLATION, process_no_json_fun},
        {TYPE_OPEN_DISTANCE_APP, process_no_json_fun},
        {TYPE_CLOSE_DISTANCE_APP, process_no_json_fun},
        {TYPE_OPEN_DAC, process_no_json_fun},
        {TYPE_CLOSE_DAC, process_no_json_fun},
        {TYPE_CLEAR_LOG, process_no_json_fun},
        {TYPE_RESET, process_no_json_fun}, 
        {TYPE_STATISTICS_INFO, process_no_json_fun},
        {TYPE_RF_INFO, process_no_json_fun},
        {TYPE_OPEN_TX_POWER, process_no_json_fun},
        {TYPE_CLOSE_TX_POWER, process_no_json_fun},
        {TYPE_OPEN_RX_GAIN, process_no_json_fun},
        {TYPE_CLOSE_RX_GAIN, process_no_json_fun},
        {TYPE_RSSI_CONTROL, process_json_fun}, 
        {TYPE_CONTROL_SAVE_CSI, process_json_fun}, 
        {TYPE_IP_SETTING, process_json_fun}, 
        {TYPE_RF_FREQ_SETTING, process_json_fun}, 
        {TYPE_OPENWRT_KEEPALIVE, fast_keepalive_response_fun}, 
        {TYPE_OPENWRT_KEEPALIVE_RESPONSE, process_keepalive_response_fun},
};

#define BUFFER_SIZE 1024 * 40
#define MAX_EVENTS 30

//void AddRecvThreadWork(int sockfd, g_server_para* g_server); // read event post to work thread process --- bug
void process_recv_event(g_receive_para* g_receive, g_server_para* g_server); // read event process in epoll loop 

void setnonblocking(int sock)
{
     int opts;
     opts=fcntl(sock,F_GETFL);
     if(opts<0)
     {
          perror("fcntl(sock,GETFL)");
          exit(1);
     }
    opts = opts|O_NONBLOCK;
     if(fcntl(sock,F_SETFL,opts)<0)
     {
          perror("fcntl(sock,SETFL,opts)");
          exit(1);
     }
}

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

int processMessage_table_drive(char* buf, int32_t length, g_receive_para* g_receive, void* pri_ptr) 
{ 
    int type = myNtohl(buf + 4);
	char* jsonfile = buf + sizeof(int32_t) + sizeof(int32_t);
    web_msg_t* msg_tmp = (web_msg_t*)malloc(sizeof(web_msg_t));
    msg_tmp->point_addr_1 = g_receive;
    parseRequestJson(jsonfile,length-4,msg_tmp);
    
    int type_num = sizeof(msg_flow) / sizeof(msg_fun_st); 
    int i = 0;
    for (i = 0; i < type_num; i++) 
    { 
        if (msg_flow[i].frame_type == type) 
        { 
            msg_flow[i].fun_ptr(type, jsonfile, length-4, msg_tmp, 0, g_receive, pri_ptr); 
            return 0; 
        } 
    }
    zlog_error(g_receive->log_handler, "unknow error frame type !!! ");
    return -1;
}

void receive(g_receive_para* g_receive, void* pri_ptr){
    int size = 0;
    int totalByte = 0;
    int msg_len = 0;
    char* temp_receBuffer = g_receive->recvbuf + 2000; //
    char* pStart = NULL;
    char* pCopy = NULL;

    size = recv(g_receive->connfd, temp_receBuffer, BUFFER_SIZE,0);
    
    if(size<=0){
		if(size < 0){
            if(errno == EAGAIN){
                // no data left in read cache buffer
                g_receive->working = NO_READ_WORK;
                //zlog_info(g_receive->log_handler, "exit receive_NO_READ_WORK +++++++++++++++++ : %d ", g_receive->connfd);
            }else{
                // error 
                zlog_error(g_receive->log_handler, "errro test 1 : errno = %d , %s \n", errno, strerror(errno));
                g_receive->working = SOCKET_CLOSE;
            }
        }
		if(size == 0){
            // couterpart socket is close
			zlog_info(g_receive->log_handler,"recv() size = 0\n");
            g_receive->working = SOCKET_CLOSE;
        }
		return;
    }

    pStart = temp_receBuffer - g_receive->moreData;
    totalByte = size + g_receive->moreData;
    const int MinHeaderLen = sizeof(int32_t);
    //zlog_info(g_receive->log_handler, " fd: %d -- receive() : test 0 --- totalByte = %d , size = %d \n", g_receive->connfd, totalByte, size);
    while(1){
        if(totalByte <= MinHeaderLen)
        {
            g_receive->moreData = totalByte;
            pCopy = pStart;
            if(g_receive->moreData > 0)
            {
                memcpy(temp_receBuffer - g_receive->moreData, pCopy, g_receive->moreData);
            }
            //zlog_info(g_receive->log_handler, " fd: %d -- receive() : test 1 --- totalByte = %d \n", g_receive->connfd, totalByte);
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
                //zlog_info(g_receive->log_handler, " fd: %d -- receive() : test 2 --- totalByte = %d \n", g_receive->connfd,totalByte);
                break;
            } 
            else// at least one message 
            {
                //zlog_info(g_receive->log_handler, " fd: %d -- receive() : test 3 --- totalByte = %d \n", g_receive->connfd,totalByte);
				int ret = processMessage_table_drive(pStart,msg_len,g_receive,pri_ptr);
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

void process_recv_event(g_receive_para* g_receive, g_server_para* g_server){
    //zlog_info(g_receive->log_handler, " process_recv_event() : start -----------------------------  \n");
    g_receive->working = WORKING;
    while(g_receive->working == WORKING){
    	receive(g_receive, (void*)g_server);
    }

    if(g_receive->working == SOCKET_CLOSE){
        postMsg(MSG_DEL_DISCONNECT_USER,NULL,0,NULL,g_receive->connfd,g_receive->g_msg_queue);
    }
    //zlog_info(g_receive->log_handler, " process_recv_event() : end -----------  \n");
}

/* epoll Server */
int unregisterEvent(int fd, g_server_para* g_server)
{
    if (epoll_ctl(g_server->epoll_node.epollfd, EPOLL_CTL_DEL, fd, NULL) == -1){
        return -1;
    }
    return 0;
}

void* runEpollServer(void* args){
    g_server_para* g_server = (g_server_para*)args;

    struct epoll_event ev,events[MAX_EVENTS];
    int nfds;
    int i;
	while(1){
        nfds = epoll_wait(g_server->epoll_node.epollfd, events, MAX_EVENTS, -1);
        if (nfds == -1) {
            zlog_error(g_server->log_handler, "epoll_wait");
            break;
        }

        for(i=0;i<nfds;i++){
            if(events[i].data.fd == g_server->listenfd){
                int connfd = -1;
                if( (connfd = accept(g_server->listenfd,(struct sockaddr*)NULL,NULL)) == -1 ){
                    continue;
                }else{
                    zlog_info(g_server->log_handler," -------------------accept new client , connfd = %d \n", connfd);

                    user_session_node* new_user = new_user_node(g_server);
                    int ret = CreateRecvParam(new_user->g_receive, g_server->g_msg_queue, connfd, g_server->log_handler);

                    setnonblocking(connfd);
                    ev.data.ptr = new_user->g_receive;
                    //ev.data.fd = connfd;
                    ev.events = EPOLLIN | EPOLLET;
                    if(epoll_ctl(g_server->epoll_node.epollfd,EPOLL_CTL_ADD,connfd,&ev) == -1){
                        zlog_error(g_server->log_handler,"epoll_ctl : connfd sock \n");
                        continue;
                    }

                    postMsg(MSG_ACCEPT_NEW_USER,NULL,0,new_user,0,g_server->g_msg_queue);
                }

            }else if(events[i].data.fd == g_server->epoll_node.event_fd){ // event_fd
                int event_fd = events[i].data.fd;
                uint64_t read_fd;
                if(event_fd & EPOLLIN){
                    if(read(event_fd, &read_fd, sizeof(read_fd)) < 0) continue;
                    //int new_fd = (int)read_fd;
                    user_session_node* new_user = (user_session_node*)read_fd;
                    int new_fd = new_user->g_receive->connfd;
                    setnonblocking(new_fd);
                    ev.data.ptr = new_user->g_receive;
                    ev.events = EPOLLIN | EPOLLET;
                    if(epoll_ctl(g_server->epoll_node.epollfd,EPOLL_CTL_ADD,new_fd,&ev) == -1){
                        zlog_error(g_server->log_handler,"epoll_ctl : read_fd sock \n");
                        continue;
                    }
                    zlog_info(g_server->log_handler,"event_fd : read_fd =  %d \n", new_fd);
                }
            }else if(events[i].events & EPOLLIN){
                // read event ready
                g_receive_para* g_receive = (g_receive_para*)(events[i].data.ptr);
                if(nfds > 2){
                    zlog_info(g_receive->log_handler, " nfds = %d ", nfds);
                }

                if ( (g_receive->connfd) < 0) continue;
                process_recv_event(g_receive,g_server);
            }
        }// for
    }// while
    zlog_info(g_server->log_handler, "Exit runEpollServer() \n");
}

/**@brief tcp连接管理网络模块
* @param[in]  g_server              网络连接管理handler
* @param[in]  g_msg_queue           消息队列handler
* @param[in]  handler               zlog
* @return  函数执行结果
* - 0          上报成功
* - 非0        上报失败
*/
int CreateServerThread(g_server_para** g_server_tmp, ThreadPool* g_threadpool, g_msg_queue_para* g_msg_queue, 
                    event_timer_t* g_timer, zlog_category_t* handler){
    *g_server_tmp = (g_server_para*)malloc(sizeof(struct g_server_para));
    g_server_para* g_server = *g_server_tmp;
    g_server->listenfd     = 0;
    g_server->g_threadpool = g_threadpool;
	g_server->g_msg_queue  = g_msg_queue;
    g_server->g_timer      = g_timer;
	g_server->log_handler  = handler;
    g_server->update_system_time = 0;
    g_server->happen_exception = 0;

    g_server->openwrt_node.openwrt_connfd = -1;
    g_server->openwrt_node.openwrt_link   = 0;
    pthread_mutex_init(&(g_server->openwrt_node.mutex),NULL);	

    INIT_LIST_HEAD(&(g_server->user_session_node_head));
	g_server->user_session_cnt    = 0;
    g_server->user_node_id_init   = 1;

    g_server->epoll_node.event_fd      = -1;
    g_server->epoll_node.epollfd       = epoll_create1(0);
    if(g_server->epoll_node.epollfd == -1){
        zlog_error(handler,"epoll_create1 error ! \n");
        return -1;
    }

    struct sockaddr_in servaddr; 
    if( (g_server->listenfd = socket(AF_INET,SOCK_STREAM,0)) == -1)
    {
        zlog_error(handler,"create socket error: %s(errno: %d)\n",strerror(errno),errno);
        return -1;
    }
 
    int one = 1;
    setsockopt(g_server->listenfd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));

    setnonblocking(g_server->listenfd);

    struct epoll_event ev;
    ev.data.fd = g_server->listenfd;
    ev.events = EPOLLIN;
    if(epoll_ctl(g_server->epoll_node.epollfd, EPOLL_CTL_ADD, g_server->listenfd, &ev) == -1){
        zlog_error(handler,"epoll_ctl : listen sock \n");
        return -1;
    }

    g_server->epoll_node.event_fd = eventfd(0, EFD_CLOEXEC|EFD_NONBLOCK);
    if(g_server->epoll_node.event_fd < 0){
        zlog_error(handler, "g_server->epoll_node.event_fd = %d ", g_server->epoll_node.event_fd);
        return -1;
    }

    struct epoll_event read_event;

    read_event.events = EPOLLHUP | EPOLLERR | EPOLLIN;
    read_event.data.fd = g_server->epoll_node.event_fd;

    if(epoll_ctl(g_server->epoll_node.epollfd, EPOLL_CTL_ADD, g_server->epoll_node.event_fd, &read_event) == -1){
        zlog_error(handler,"epoll_ctl : epoll_node.event_fd  \n");
        return -1;
    }

    memset(&servaddr,0,sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(TCP_LISTEN_PORT);
 
    if( bind(g_server->listenfd,(struct sockaddr*)&servaddr,sizeof(servaddr)) == -1)
    {
        zlog_error(handler,"bind socket error: %s(errno: %d)\n",strerror(errno),errno);
        return -1;
    }

    if( listen(g_server->listenfd,16) == -1) // max number of user in same time
    {
        zlog_error(handler,"listen socket error: %s(errno: %d)\n",strerror(errno),errno);
        return -1;
    }

    AddWorker(runEpollServer,(void*)g_server,g_threadpool);

	return 0;
}

void set_recv_working_state(int state, g_receive_para* g_receive){
    pthread_mutex_lock(&(g_receive->working_mutex));
    g_receive->working = state;
    pthread_mutex_unlock(&(g_receive->working_mutex));
}

int get_recv_working_state(g_receive_para* g_receive){
    int ret;
    pthread_mutex_lock(&(g_receive->working_mutex));
    ret = g_receive->working;
    pthread_mutex_unlock(&(g_receive->working_mutex));
    return ret;
}

/*  epoll call in accept event */
int CreateRecvParam(g_receive_para* g_receive, g_msg_queue_para* g_msg_queue, int connfd, zlog_category_t* handler){
	//zlog_info(handler,"CreateRecvParam()");
	g_receive->g_msg_queue     = g_msg_queue;
	g_receive->connfd          = connfd;                     // connfd
	g_receive->log_handler 	   = handler;
	g_receive->moreData        = 0;
    g_receive->recvbuf         = (char*)malloc(BUFFER_SIZE);
    g_receive->sendbuf         = (char*)malloc(BUFFER_SIZE);
    pthread_mutex_init(&(g_receive->send_mutex),NULL);
    g_receive->working         = NO_READ_WORK;
    pthread_mutex_init(&(g_receive->working_mutex),NULL);	
	return 0;
}

/* ------------------------- user session --------------------------------- */
void add_new_user_node_to_list(user_session_node* new_node, g_server_para* g_server){
    new_node->node_id = g_server->user_node_id_init;
    list_add_tail(&new_node->list, &g_server->user_session_node_head);
    g_server->user_session_cnt++;
    g_server->user_node_id_init++;
    if(g_server->user_node_id_init == 1025){
        g_server->user_node_id_init = 1;
    }
}


user_session_node* new_user_node(g_server_para* g_server){
    user_session_node* new_node = (user_session_node*)malloc(sizeof(user_session_node));
    new_node->g_receive = (g_receive_para*)malloc(sizeof(g_receive_para));
    new_node->record_action = (record_action_t*)malloc(sizeof(record_action_t));

    new_node->record_action->enable_rssi = 0;
    new_node->record_action->enable_rssi_save = 0;

    new_node->record_action->enable_start_csi = 0;
    new_node->record_action->enable_csi_save = 0;

    new_node->record_action->enable_start_constell = 0;
    new_node->user_ip = NULL;

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
    free(g_receive->sendbuf);
    free(g_receive->recvbuf);
    free(g_receive);
    g_receive = NULL;
}

/* ------------------------- send interface-------------------------------- */
/* all call in event loop may be? */

/**@brief     后端发送消息接口
* @param[in]  g_receive              对应不同的连接接收handler(前端发送服务请求到后端，该请求对应不同的用户，后端需根据该handler正确回复请求的发起者)
* @param[in]  buf                    消息buffer
* @param[in]  buf_len                消息buffer长度（字符串不包括字符串结束符）
* @param[in]  type                   回复前端消息类型 ( for debug )
* @return  函数执行结果
* - 0          上报成功
* - 非0        上报失败
*/
/* 待修改 ：sendbuf使用临时的 */
int assemble_frame_and_send(g_receive_para* g_receive, char* buf, int buf_len, int type){
    //zlog_info(g_receive->log_handler," buf : %s",buf);
    if(g_receive == NULL){
        return -99;
    }
    if(get_recv_working_state(g_receive) == SOCKET_CLOSE){
        zlog_info(g_receive->log_handler, "Receive Working State is SOCKET_CLOSE ! type = %d ", type);
        return -98;
    }
    int length = buf_len + FRAME_HEAD_ROOM;
    pthread_mutex_lock(&(g_receive->send_mutex));
    char* temp_buf = g_receive->sendbuf;
    *((int32_t*)temp_buf) = htonl(buf_len + sizeof(int32_t));
    *((int32_t*)(temp_buf + sizeof(int32_t))) = htonl(type);
    if(buf_len > 0){
        memcpy(temp_buf + FRAME_HEAD_ROOM,buf,buf_len);
    }
    int ret = send(g_receive->connfd, temp_buf, length, 0);
    if(ret != length){
        zlog_info(g_receive->log_handler,"ret = %d" , ret);
        pthread_mutex_unlock(&(g_receive->send_mutex));
        return -1;
    }

    if(TYPE_SYSTEM_STATE_RESPONSE == type || TYPE_SYSTEM_STATE_EXCEPTION == type){
        //zlog_info(g_receive->log_handler, "system state json : %s ", buf);
    }

    pthread_mutex_unlock(&(g_receive->send_mutex));
    return ret;
}


