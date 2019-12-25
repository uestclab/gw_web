#include "cJSON.h"
#include "event_process.h"
#include "web_common.h"
#include "small_utility.h"

/* test */
#include "response_json.h"
#include "md5sum.h"

void* inquiry_reg_state_loop(void* args){
	g_receive_para* tmp_receive = (g_receive_para*)args;
	sleep(1);
	struct msg_st data;
	data.msg_type = MSG_INQUIRY_REG_STATE;
	data.msg_number = MSG_INQUIRY_REG_STATE;
	data.tmp_data = tmp_receive;
	data.msg_len = 0;
	postMsgQueue(&data,tmp_receive->g_msg_queue);
}

void postTmpWorkToThreadPool(g_receive_para* tmp_receive, ThreadPool* g_threadpool){
	AddWorker(inquiry_reg_state_loop,(void*)tmp_receive,g_threadpool);
}

void display(g_server_para* g_server){
	zlog_info(g_server->log_handler,"  ---------------- display () --------------------------\n");
	zlog_info(g_server->log_handler,"user_session_cnt = %d " , g_server->user_session_cnt);
	zlog_info(g_server->log_handler,"  ---------------- end display () ----------------------\n");
}

/* ------------------ test dynamic change conf ------------------------- */
void* monitor_conf_thread(void* args){

	pthread_detach(pthread_self());

	g_broker_para* g_broker = (g_broker_para*)args;
	char* conf_path = "../conf/test_conf.json";
	char* p_conf_file = readfile(conf_path);
	if(p_conf_file == NULL){
		zlog_error(g_broker->log_handler,"open file %s error.\n",conf_path);
		return NULL;
	}
	zlog_info(g_broker->log_handler, "conf : %s \n", p_conf_file);
	free(p_conf_file);
	int state = 0;

	unsigned char old_sum[16];
	state = get_md5sum(old_sum, conf_path);

	while(1){
		sleep(5);
		unsigned char sum[16];
		state = get_md5sum(sum, conf_path);
		int ret = memcmp(old_sum,sum,16);
		if(ret != 0){
			postMsg(MSG_CONF_CHANGE,conf_path,strlen(conf_path)+1,NULL,0,g_broker->g_msg_queue);
			memcpy(old_sum,sum,16);
		}
	}
}

void create_monitor_configue_change(g_broker_para* g_broker){
	pthread_t thread_pid;
	pthread_create(&thread_pid, NULL, monitor_conf_thread, (void*)(g_broker));
}

/* -------------------------- main process msg loop --------------------------------------------- */

void eventLoop(g_server_para* g_server, g_broker_para* g_broker, g_dma_para* g_dma, g_msg_queue_para* g_msg_queue, ThreadPool* g_threadpool, zlog_category_t* zlog_handler)
{
	create_monitor_configue_change(g_broker);
	while(1){
		struct msg_st* getData = getMsgQueue(g_msg_queue);
		if(getData == NULL)
			continue;
		
		switch(getData->msg_type){
			case MSG_ACCEPT_NEW_USER:
			{
				zlog_info(zlog_handler," ---------------- EVENT : MSG_ACCEPT_NEW_USER: msg_number = %d",getData->msg_number);

				int connfd = *((int*)(getData->tmp_data));
				free(getData->tmp_data);

				user_session_node* new_user = new_user_node(g_server);
	
				int ret = CreateRecvThread(new_user->g_receive, g_server->g_msg_queue, connfd, g_server->log_handler);


				if(g_broker->enableCallback == 0){
					zlog_info(zlog_handler," ---------------- EVENT : MSG_ACCEPT_NEW_USER: register broker callback \n");
					broker_register_callback_interface(g_broker);
					g_broker->enableCallback = 1;
				}

				if(g_dma->enableCallback == 0){
					zlog_info(zlog_handler, "---------------- EVENT : MSG_ACCEPT_NEW_USER: register dma callback \n");
					dma_register_callback(g_dma);
					g_dma->enableCallback = 1;
				}

				display(g_server);

				break;
			}
			/* clear one user all status */
			case MSG_RECEIVE_THREAD_CLOSED:
			{
				zlog_info(zlog_handler," ---------------- EVENT : MSG_RECEIVE_THREAD_CLOSED: msg_number = %d",getData->msg_number);

				g_receive_para* tmp_receive = (g_receive_para*)getData->tmp_data;
				
				del_user(tmp_receive->connfd, g_server, g_broker, g_dma, g_threadpool);

				display(g_server);

				break;
			}
			case MSG_INQUIRY_SYSTEM_STATE:
			{
				zlog_info(zlog_handler," ---------------- EVENT : MSG_INQUIRY_SYSTEM_STATE: msg_number = %d",getData->msg_number);

				g_receive_para* tmp_receive = (g_receive_para*)getData->tmp_data;

				inquiry_system_state(tmp_receive,g_broker);
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
				//zlog_info(zlog_handler," ---------------- EVENT : MSG_INQUIRY_REG_STATE: msg_number = %d",getData->msg_number);
				
				g_receive_para* tmp_receive = (g_receive_para*)getData->tmp_data;

				inquiry_reg_state(tmp_receive, g_broker);
				break;
			}
			case MSG_INQUIRY_RSSI: // open rssi
			{
				zlog_info(zlog_handler," ---------------- EVENT : MSG_INQUIRY_RSSI: msg_number = %d",getData->msg_number);
				g_receive_para* tmp_receive = (g_receive_para*)getData->tmp_data;
				/* open rssi */
				int ret = open_rssi_state_external(tmp_receive->connfd, g_broker);
				/* record open rssi : when check return */
				if(ret == 0){
					record_rssi_enable(tmp_receive->connfd, g_server);
				}

				break;
			}
			case MSG_RSSI_READY_AND_SEND:
			{
				//zlog_info(zlog_handler," ---------------- EVENT : MSG_RSSI_READY_AND_SEND: msg_number = %d",getData->msg_number);

				/* send rssi to node.js for display */
				send_rssi_in_event_loop(getData->msg_json, getData->msg_len, g_broker);

				/* check and save rssi to file */
				send_rssi_to_save(getData->msg_json, getData->msg_len, g_broker);

				break;
			}
			case MSG_CONTROL_RSSI: // rssi save enable or not
			{
				zlog_info(zlog_handler," ---------------- EVENT : MSG_CONTROL_RSSI: msg_number = %d",getData->msg_number);
				g_receive_para* tmp_receive = (g_receive_para*)getData->tmp_data;

				int ret = process_rssi_save_file(tmp_receive->connfd, getData->msg_json,getData->msg_len,g_broker);
				/* record rssi save cmd : if check return of process_rssi_save_file() ?*/
				if(ret == 0){
					record_rssi_save_enable(tmp_receive->connfd, getData->msg_json, getData->msg_len, g_server);
				}
				/* inform node js cmd state */
				send_cmd_state(tmp_receive ,ret);
				break;
			}
			case MSG_CLEAR_RSSI_WRITE_STATUS: // case 2 : if not close save rssi manually, this event must be behind MSG_RECEIVE_THREAD_CLOSED
			{
				zlog_info(zlog_handler," ---------------- EVENT : MSG_CLEAR_RSSI_WRITE_STATUS: msg_number = %d",getData->msg_number);

				rssi_user_node* tmp_node = (rssi_user_node*)getData->tmp_data;

				clear_rssi_write_status(tmp_node,g_broker);

				break;
			}
			case MSG_INQUIRY_RF_MF_STATE:
			{
				zlog_info(zlog_handler," ---------------- EVENT : MSG_INQUIRY_RF_MF_STATE: msg_number = %d",getData->msg_number);
				break;
			}
			case MSG_CONF_CHANGE: // for self test
			{
				zlog_info(zlog_handler," ---------------- EVENT : MSG_CONF_CHANGE: msg_number = %d",getData->msg_number);

				char* p_conf_file = readfile(getData->msg_json);
				if(p_conf_file == NULL){
					zlog_error(g_broker->log_handler,"open file %s error.\n",getData->msg_json);
					break;
				}
				zlog_info(g_broker->log_handler, "new conf : %s \n", p_conf_file);

				cJSON * root = NULL;
    			cJSON * item = NULL;
    			root = cJSON_Parse(p_conf_file);
    			item = cJSON_GetObjectItem(root,"id");
				int state_id = item->valueint;
				item = cJSON_GetObjectItem(root,"cmd");
				int cmd = item->valueint;

				struct user_session_node *pnode = NULL;
				list_for_each_entry(pnode, &g_server->user_session_node_head, list) {
					if(pnode->g_receive != NULL){
						if(state_id == 11 && cmd == 11){
							postMsg(MSG_OPEN_DISTANCE_APP,NULL,0,pnode->g_receive,0,g_broker->g_msg_queue);
						}else if(state_id == 22 && cmd == 22){
							postMsg(MSG_CLOSE_DISTANCE_APP,NULL,0,pnode->g_receive,0,g_broker->g_msg_queue);
						}else if(state_id == 33 && cmd == 33){
							postMsg(MSG_OPEN_DAC,NULL,0,pnode->g_receive,0,g_broker->g_msg_queue);
						}else if(state_id == 44 && cmd == 44){
							postMsg(MSG_CLOSE_DAC,NULL,0,pnode->g_receive,0,g_broker->g_msg_queue);
						}else if(state_id == 99 && cmd == 99){
							postMsg(MSG_CLEAR_LOG,NULL,0,pnode->g_receive,0,g_broker->g_msg_queue);
						}				
					}
				}


				//test_process_exception(state_cnt, g_broker);
				cJSON_Delete(root);
				free(p_conf_file);

				break;
			}
			case MSG_START_CSI:
			{
				zlog_info(zlog_handler," ---------------- EVENT : MSG_START_CSI: msg_number = %d",getData->msg_number);

				g_receive_para* tmp_receive = (g_receive_para*)getData->tmp_data;

				/* check mutex with constell */
				int state = check_constell_working(g_server);
				if(state == 1){
					send_cmd_state(tmp_receive ,CSI_CONSTELL_MUTEX);
					break;
				}

				state = start_csi_state_external(tmp_receive->connfd,g_dma);

				if(state == 0){
					record_csi_start_enable(tmp_receive->connfd, START, g_server);
				}

				/* inform node js cmd state */
				send_cmd_state(tmp_receive ,state);

				break;
			}
			case MSG_STOP_CSI:
			{
				zlog_info(zlog_handler," ---------------- EVENT : MSG_STOP_CSI: msg_number = %d",getData->msg_number);

				g_receive_para* tmp_receive = (g_receive_para*)getData->tmp_data;

				stop_csi_state_external(tmp_receive->connfd, g_dma);

				record_csi_start_enable(tmp_receive->connfd, STOP, g_server);

				/* inform node js cmd state */
				send_cmd_state(tmp_receive,CMD_OK);

				break;
			}
			case MSG_CSI_READY:
			{
				//zlog_info(zlog_handler," ---------------- EVENT : MSG_CSI_READY: msg_number = %d",getData->msg_number);

				if(getData->msg_len != 1024){
					zlog_info(zlog_handler," getData->msg_len != 1024 : %d ",getData->msg_len);
					break;
				}

				processCSI(getData->msg_json, 1024, g_dma);
				
				/* send to display */
				send_csi_display_in_event_loop(g_dma);

				/* send to save */
				send_csi_to_save(g_dma);

				break;
			}
			case MSG_START_CONSTELLATION:
			{
				zlog_info(zlog_handler," ---------------- EVENT : MSG_START_CONSTELLATION: msg_number = %d",getData->msg_number);

				g_receive_para* tmp_receive = (g_receive_para*)getData->tmp_data;

				/* check mutex with csi */
				int state = check_csi_working(g_server);
				if(state == 1){
					send_cmd_state(tmp_receive ,CSI_CONSTELL_MUTEX);
					break;
				}

				start_constellation_external(tmp_receive->connfd,g_dma);

				record_constell_start_enable(tmp_receive->connfd, START, g_server);

				send_cmd_state(tmp_receive ,CMD_OK);

				break;
			}
			case MSG_STOP_CONSTELLATION:
			{
				zlog_info(zlog_handler," ---------------- EVENT : MSG_STOP_CONSTELLATION: msg_number = %d",getData->msg_number);

				g_receive_para* tmp_receive = (g_receive_para*)getData->tmp_data;

				stop_constellation_external(tmp_receive->connfd, g_dma);

				record_constell_start_enable(tmp_receive->connfd, STOP, g_server);

				send_cmd_state(tmp_receive ,CMD_OK);

				break;
			}
			case MSG_CONSTELLATION_READY:
			{
				//zlog_info(zlog_handler," ---------------- EVENT : MSG_CONSTELLATION_READY: msg_number = %d",getData->msg_number);

				/* process IQ data */
				processConstellation(getData->tmp_data, getData->tmp_data_len, g_dma);

				/* send IQ data to display */
				send_constell_display_in_event_loop(g_dma);

				break;
			}
			case MSG_CONTROL_SAVE_IQ_DATA:
			{
				zlog_info(zlog_handler," ---------------- EVENT : MSG_CONTROL_SAVE_IQ_DATA: msg_number = %d",getData->msg_number);

				g_receive_para* tmp_receive = (g_receive_para*)getData->tmp_data;

				process_csi_save_file(tmp_receive->connfd, getData->msg_json,getData->msg_len, g_dma);

				record_csi_save_enable(tmp_receive->connfd, getData->msg_json, getData->msg_len, g_server);

				send_cmd_state(tmp_receive ,CMD_OK);

				break;
			}
			case MSG_CLEAR_CSI_WRITE_STATUS:
			{
				zlog_info(zlog_handler," ---------------- EVENT : MSG_CLEAR_CSI_WRITE_STATUS: msg_number = %d",getData->msg_number);

				csi_save_user_node* tmp_node = (csi_save_user_node*)getData->tmp_data;

				clear_csi_write_status(tmp_node,g_dma);

				break;
			}
			case MSG_OPEN_DISTANCE_APP:
			{
				zlog_info(zlog_handler," ---------------- EVENT : MSG_OPEN_DISTANCE_APP: msg_number = %d",getData->msg_number);
				g_receive_para* tmp_receive = (g_receive_para*)getData->tmp_data;
				system("sh /tmp/gw_app/distance/conf/open_app.sh");
				send_cmd_state(tmp_receive ,CMD_OK);
				break;
			}
			case MSG_CLOSE_DISTANCE_APP:
			{
				zlog_info(zlog_handler," ---------------- EVENT : MSG_CLOSE_DISTANCE_APP: msg_number = %d",getData->msg_number);
				g_receive_para* tmp_receive = (g_receive_para*)getData->tmp_data;
				system("sh /tmp/gw_app/distance/conf/close_app.sh");
				send_cmd_state(tmp_receive ,CMD_OK);
				break;
			}
			case MSG_OPEN_DAC:
			{
				zlog_info(zlog_handler," ---------------- EVENT : MSG_OPEN_DAC: msg_number = %d",getData->msg_number);
				g_receive_para* tmp_receive = (g_receive_para*)getData->tmp_data;
				system("echo 1 > /sys/class/gpio/gpio973/value");
				send_cmd_state(tmp_receive ,CMD_OK);
				break;
			}
			case MSG_CLOSE_DAC:
			{
				zlog_info(zlog_handler," ---------------- EVENT : MSG_CLOSE_DAC: msg_number = %d",getData->msg_number);
				g_receive_para* tmp_receive = (g_receive_para*)getData->tmp_data;
				system("echo 0 > /sys/class/gpio/gpio973/value");
				send_cmd_state(tmp_receive ,CMD_OK);
				break;
			}
			case MSG_CLEAR_LOG:
			{
				zlog_info(zlog_handler," ---------------- EVENT : MSG_CLEAR_LOG: msg_number = %d",getData->msg_number);
				g_receive_para* tmp_receive = (g_receive_para*)getData->tmp_data;
				system("sh /tmp/web/conf/clear_log.sh");
				send_cmd_state(tmp_receive ,CMD_OK);
				break;
			}
			default:
				break;
		}// end switch
		free(getData);
	}// end while(1)
}


/* -------------------- event process function --------------------------- */

void del_user(int connfd, g_server_para* g_server, g_broker_para* g_broker, g_dma_para* g_dma, ThreadPool* g_threadpool){
	user_session_node* tmp_node = del_user_node_in_list(connfd, g_server);
	if(tmp_node == NULL){
		return;
	}

	// process action
	if(tmp_node->record_action->enable_rssi_save){
		/* disable save rssi */
		inform_stop_rssi_write_thread(connfd, g_broker);
	}
	if(tmp_node->record_action->enable_rssi){
		/* close rssi if need */
		close_rssi_state_external(connfd, g_broker);
	}
	if(tmp_node->record_action->enable_start_csi){
		/* stop csi if need */
		stop_csi_state_external(connfd, g_dma);
	}
	if(tmp_node->record_action->enable_csi_save){
		/* disable save csi */
		inform_stop_csi_write_thread(connfd, g_dma);
	}
	if(tmp_node->record_action->enable_start_constell){
		/* stop constellation if need */
		stop_constellation_external(connfd, g_dma);
	}

	// free node
	release_receive_resource(tmp_node->g_receive);
	free(tmp_node->record_action);
	free(tmp_node);
}

/* ------------------------------ rssi record function ---------------------------- */

void record_rssi_enable(int connfd, g_server_para* g_server){
    struct user_session_node *pnode = NULL;
    list_for_each_entry(pnode, &g_server->user_session_node_head, list) {
        if(pnode->g_receive->connfd == connfd){    
            pnode->record_action->enable_rssi = 1;
			zlog_info(g_server->log_handler,"connfd = %d , enable_rssi = 1 \n", connfd);
            break;
        }
    }
}

int record_rssi_save_enable(int connfd, char* stat_buf, int stat_buf_len, g_server_para* g_server){

	cJSON * root = NULL;
    cJSON * item = NULL;
    root = cJSON_Parse(stat_buf);
    item = cJSON_GetObjectItem(root,"type");
	if(item->valueint != TYPE_RSSI_CONTROL){
		cJSON_Delete(root);
		return -1;
	}

	int rssi_save = -1;
	item = cJSON_GetObjectItem(root,"op_cmd");
	if(item->valueint == 0){ /* stop save */
		rssi_save = 0;
	}else if(item->valueint == 1){ /* start save */
		rssi_save = 1;
	}

    struct user_session_node *pnode = NULL;
    list_for_each_entry(pnode, &g_server->user_session_node_head, list) {
        if(pnode->g_receive->connfd == connfd){    
            pnode->record_action->enable_rssi_save = rssi_save;
			zlog_info(g_server->log_handler,"connfd = %d , enable_rssi_save = %d \n", connfd, rssi_save);
            break;
        }
    }

	cJSON_Delete(root);
	return 0;
}

/* --------------------------------- csi record function ------------------------------------------ */

void record_csi_start_enable(int connfd, int enable, g_server_para* g_server){
	struct user_session_node *pnode = NULL;
	list_for_each_entry(pnode, &g_server->user_session_node_head, list) {
		if(pnode->g_receive->connfd == connfd){
			pnode->record_action->enable_start_csi = enable;
			zlog_info(g_server->log_handler,"connfd = %d , enable_start_csi = %d \n", connfd, enable);
            break;
		}
	}
}

int record_csi_save_enable(int connfd, char* stat_buf, int stat_buf_len, g_server_para* g_server){

	cJSON * root = NULL;
    cJSON * item = NULL;
    root = cJSON_Parse(stat_buf);
    item = cJSON_GetObjectItem(root,"type");
	if(item->valueint != TYPE_CONTROL_SAVE_CSI){
		cJSON_Delete(root);
		return -1;
	}

	int csi_save = -1;
	item = cJSON_GetObjectItem(root,"op_cmd");
	if(item->valueint == 0){ /* stop save */
		csi_save = 0;
	}else if(item->valueint == 1){ /* start save */
		csi_save = 1;
	}

    struct user_session_node *pnode = NULL;
    list_for_each_entry(pnode, &g_server->user_session_node_head, list) {
        if(pnode->g_receive->connfd == connfd){    
            pnode->record_action->enable_csi_save = csi_save;
			zlog_info(g_server->log_handler,"connfd = %d , enable_csi_save = %d \n", connfd, csi_save);
            break;
        }
    }

	cJSON_Delete(root);
	return 0;
}

/* --------------------------------- constellation record function ------------------------------------------ */
void record_constell_start_enable(int connfd, int enable, g_server_para* g_server){
	struct user_session_node *pnode = NULL;
	list_for_each_entry(pnode, &g_server->user_session_node_head, list) {
		if(pnode->g_receive->connfd == connfd){
			pnode->record_action->enable_start_constell = enable;
			zlog_info(g_server->log_handler,"connfd = %d , enable_start_constell = %d \n", connfd, enable);
            break;
		}
	}
}


/* --------------------------  cmd CMD_OK or CMD_FAIL and mutex check between csi and constellation ----------------------------------*/
// #define CMD_OK 0
// #define CMD_FAIL -1
// #define CSI_CONSTELL_MUTEX 2
void send_cmd_state(g_receive_para* g_receive ,int state){
	int cmd_state = CMD_FAIL;
	if(state == CMD_OK){
		cmd_state = CMD_OK;
	}else if(state == CSI_CONSTELL_MUTEX){
		cmd_state = CSI_CONSTELL_MUTEX;
	}
	
	char *cmd_state_response_json = cmd_state_response(cmd_state);
	assemble_frame_and_send(g_receive,cmd_state_response_json,strlen(cmd_state_response_json),TYPE_CMD_STATE_RESPONSE);
	free(cmd_state_response_json);
}

int check_constell_working(g_server_para* g_server){
	int state = -1;
    struct user_session_node *tmp_node = NULL;
    list_for_each_entry(tmp_node, &g_server->user_session_node_head, list) {
		state = tmp_node->record_action->enable_start_constell;
		if(state == 1){
			break;
		}
    }
	return state;
}

int check_csi_working(g_server_para* g_server){
	int state = -1;
    struct user_session_node *tmp_node = NULL;
    list_for_each_entry(tmp_node, &g_server->user_session_node_head, list) {
		state = tmp_node->record_action->enable_start_csi;
		if(state == 1){
            break;
        }
    }
	return state;
}

