#include "event_process.h"
#include "cJSON.h"
#include "web_common.h"

void* inquiry_reg_state_loop(void* args){
	g_server_para* g_server = (g_server_para*)args;
	sleep(1);
	struct msg_st data;
	data.msg_type = MSG_INQUIRY_REG_STATE;
	data.msg_number = MSG_INQUIRY_REG_STATE;
	data.msg_len = 0;
	postMsgQueue(&data,g_server->g_msg_queue);
}

void postTmpWorkToThreadPool(g_server_para* g_server, ThreadPool* g_threadpool){
	AddWorker(inquiry_reg_state_loop,(void*)g_server,g_threadpool);
}

void display(g_server_para* g_server){
	zlog_info(g_server->log_handler,"  ---------------- display () --------------------------\n");

	zlog_info(g_server->log_handler,"  ---------------- end display () ----------------------\n");
}

/* -------------------------- main process msg loop --------------------------------------------- */

void eventLoop(g_server_para* g_server, g_broker_para* g_broker, g_msg_queue_para* g_msg_queue, ThreadPool* g_threadpool, zlog_category_t* zlog_handler)
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


				if(g_broker->enableCallback == 0){
					zlog_info(zlog_handler," ---------------- EVENT : MSG_ACCEPT_NEW_CLIENT: register callback \n");
					broker_register_callback_interface(g_broker);
					//dma_register_callback(g_dma);
					g_broker->enableCallback = 1;
				}

				/* open rssi */
				control_rssi_state(g_broker->json_set.rssi_open_json,strlen(g_broker->json_set.rssi_open_json), g_broker);

				break;
			}
			case MSG_RECEIVE_THREAD_CLOSED:
			{
				zlog_info(zlog_handler," ---------------- EVENT : MSG_RECEIVE_THREAD_CLOSED: msg_number = %d",getData->msg_number);
				g_server->has_user = 0;
				close(g_server->g_receive_var.connfd);

				/* close rssi if need */
				control_rssi_state(g_broker->json_set.rssi_close_json,strlen(g_broker->json_set.rssi_close_json), g_broker);

				break;
			}
			case MSG_INQUIRY_SYSTEM_STATE:
			{
				zlog_info(zlog_handler," ---------------- EVENT : MSG_INQUIRY_SYSTEM_STATE: msg_number = %d",getData->msg_number);

				inquiry_system_state(g_broker);
				break;
			}
			case MSG_SYSTEM_STATE_EXCEPTION: // clear system state variable
			{
				zlog_info(zlog_handler," ---------------- EVENT : MSG_SYSTEM_STATE_EXCEPTION: msg_number = %d",getData->msg_number);

				process_exception(getData->msg_json,getData->msg_len,g_broker);

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

				inquiry_reg_state(g_broker);
/* ------------------------------ test code -----------------------------------  */
				if(g_server->has_user != 0){
					postTmpWorkToThreadPool(g_server, g_threadpool);
				}
/* ------------------------------ test code -----------------------------------  */
				break;
			}
			case MSG_INQUIRY_RSSI:
			{
				zlog_info(zlog_handler," ---------------- EVENT : MSG_INQUIRY_RSSI: msg_number = %d",getData->msg_number);
				break;
			}
			case MSG_CONTROL_RSSI:
			{
				zlog_info(zlog_handler," ---------------- EVENT : MSG_CONTROL_RSSI: msg_number = %d",getData->msg_number);
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






