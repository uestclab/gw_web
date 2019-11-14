#include "event_process.h"
#include "cJSON.h"
#include "web_common.h"

typedef void (*timercb_func)(g_msg_queue_para* g_msg_queue);
void timerout_cb(g_msg_queue_para* g_msg_queue){
	struct msg_st data;
	data.msg_type = MSG_TIMEOUT;
	data.msg_number = MSG_TIMEOUT;
	data.msg_len = 0;
	postMsgQueue(&data,g_msg_queue);
}


void display(g_server_para* g_server){
	zlog_info(g_server->log_handler,"  ---------------- display () --------------------------\n");

	zlog_info(g_server->log_handler,"  ---------------- end display () ----------------------\n");
}

void eventLoop(g_server_para* g_server, g_msg_queue_para* g_msg_queue, zlog_category_t* zlog_handler)
{
	while(1){
		struct msg_st* getData = getMsgQueue(g_msg_queue);
		if(getData == NULL)
			continue;
		
		switch(getData->msg_type){
			case MSG_ACCEPT_NEW_USER:
			{
				zlog_info(zlog_handler," ---------------- EVENT : MSG_ACCEPT_NEW_USER: msg_number = %d",getData->msg_number);

				int connfd = -1;
				if(getData->msg_len > 0){
					memcpy((char*)(&connfd),getData->msg_json,getData->msg_len);
				}

				if(g_server->has_user == 1){
					zlog_info(g_server->log_handler,"already in use web service \n");
					close(connfd);
					continue;
				}

				//g_receive_para* g_receive = NULL;
				int ret = CreateRecvThread(&(g_server->g_receive_var), g_server->g_msg_queue, connfd, g_server->log_handler);
				g_server->has_user  = 1;

				break;
			}
			case MSG_RECEIVE_THREAD_CLOSED:
			{
				zlog_info(zlog_handler," ---------------- EVENT : MSG_RECEIVE_THREAD_CLOSED: msg_number = %d",getData->msg_number);
				g_server->has_user = 0;
				close(g_server->g_receive_var.connfd);
				break;
			}
			case MSG_TIMEOUT:
			{
				zlog_info(zlog_handler," ---------------- EVENT : MSG_TIMEOUT: msg_number = %d",getData->msg_number);
				break;
			}
			case MSG_INQUIRY_REG_STATE:
			{
				zlog_info(zlog_handler," ---------------- EVENT : MSG_INQUIRY_REG_STATE: msg_number = %d",getData->msg_number);
				break;
			}
			case MSG_INQUIRY_RF_MF_STATE:
			{
				zlog_info(zlog_handler," ---------------- EVENT : MSG_INQUIRY_RF_MF_STATE: msg_number = %d",getData->msg_number);
				break;
			}
			default:
				break;
		}// end switch
		free(getData);
	}// end while(1)
}






