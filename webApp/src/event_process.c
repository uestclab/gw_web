#include "cJSON.h"
#include "event_process.h"
#include "web_common.h"
#include "small_utility.h"
#include "auto_log.h"
#include "sys_handler.h"
#include "rf_module.h"
#include "threadFuncWrapper.h"
#include "timer.h"

/* test */
#include "response_json.h"
#include "md5sum.h"
#include "utility.h"

void monitorManageInfo(g_server_para* g_server, g_broker_para* g_broker, g_dma_para* g_dma){
	zlog_category_t* log_handler = g_server->log_handler;
	zlog_info(log_handler,"  ---------------- displayManageInfo () --------------------------\n");
	
	zlog_info(log_handler,"user_node_cnt = %d \n" , g_server->user_session_cnt);
	zlog_info(log_handler,"rssi user_cnt = %d \n", g_broker->rssi_module.user_cnt);
	zlog_info(log_handler,"csi user_cnt = %d \n",g_dma->csi_module.user_cnt);
	zlog_info(log_handler,"csi save_user_cnt = %d \n",g_dma->csi_module.save_user_cnt);
	zlog_info(log_handler,"constellation user_cnt = %d \n",g_dma->constellation_module.user_cnt);

	struct timeval tv;
  	gettimeofday(&tv, NULL);
	int64_t end = tv.tv_sec;
	double acc_sec = end-g_broker->start_time;
	zlog_info(log_handler, "start = %lld , end = %lld , acc_sec = %lf \n", g_broker->start_time, end, acc_sec);
	
	zlog_info(log_handler,"  ---------------- end displayManageInfo () ----------------------\n");
}

void display(g_server_para* g_server){
	zlog_info(g_server->log_handler,"  ---------------- display () --------------------------\n");
	zlog_info(g_server->log_handler,"user_session_cnt = %d " , g_server->user_session_cnt);

	int user_cnt = 1;
	struct user_session_node *pnode = NULL;
	list_for_each_entry(pnode, &g_server->user_session_node_head, list) {
		if(pnode->g_receive != NULL){    
			zlog_info(g_server->log_handler,"-- %d : user fd =  %d " , user_cnt, pnode->g_receive->connfd);
			user_cnt = user_cnt + 1;
		}
	}

	zlog_info(g_server->log_handler,"  ---------------- end display () ----------------------\n");
}

/* ------------------ link detect ----------------------------- */
void* link_detect_thread(void* args){
	g_server_para* g_server = (g_server_para*)args;
	int net_stat = -1;
	while(1){
		net_stat = get_netlink_status("eth0");
		//zlog_info(g_server->log_handler, "Net link status: %d\n", net_stat);
		if(net_stat == 1 && g_server->openwrt_link == 0){
			int connfd = connect_helloworld();
			if(connfd > 0){
				g_server->openwrt_link = 1;
				g_server->openwrt_connfd = connfd;
				postMsg(MSG_OPENWRT_CONNECTED,NULL,0,NULL,0,g_server->g_msg_queue);
			}
		}else if(net_stat == 0 && g_server->openwrt_link == 1){
			g_server->openwrt_link = 0;
			postMsg(MSG_OPENWRT_DISCONNECT,NULL,0,NULL,0,g_server->g_msg_queue);
		}
		sleep(2);
	}
}

void create_link_detect(g_server_para* g_server){
	AddWorker(link_detect_thread,(void*)g_server,g_server->g_threadpool);
}


/* ------------------ test dynamic change conf ------------------------- */
void* monitor_conf_thread(void* args){
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

void create_monitor_configue_change(g_broker_para* g_broker, ThreadPool* g_threadpool){
	AddWorker(monitor_conf_thread,(void*)g_broker,g_threadpool);
}

/* -------------------------- main process msg loop --------------------------------------------- */
void 
eventLoop(g_server_para* g_server, g_broker_para* g_broker, g_dma_para* g_dma, 
		g_msg_queue_para* g_msg_queue, ThreadPool* g_threadpool, event_timer_t* g_timer, zlog_category_t* zlog_handler)
{

	record_str_t* g_record = (record_str_t*)malloc(sizeof(record_str_t));

	init_record_str(g_record);
	if(auto_log_start(&logc,zlog_handler) != 0){
		return;
	}

	addTimeOutWorkToTimer(g_msg_queue,g_timer);

	//int num = 0;
	//addLogTaskToTimer(g_msg_queue, &num, g_timer);
	create_link_detect(g_server);
	create_monitor_configue_change(g_broker, g_threadpool);

	while(1){
		struct msg_st* getData = getMsgQueue(g_msg_queue);
		if(getData == NULL){
			zlog_info(zlog_handler," getMsgQueue : getData == NULL \n");
			continue;
		}
		web_msg_t* tmp_web = NULL;
		switch(getData->msg_type){
			case MSG_ACCEPT_NEW_USER:
			{
				zlog_info(zlog_handler," ---------------- EVENT : MSG_ACCEPT_NEW_USER: msg_number = %d",getData->msg_number);

				user_session_node* new_node = (user_session_node*)getData->tmp_data;
				add_new_user_node_to_list(new_node, g_server);

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
			case MSG_DEL_DISCONNECT_USER:
			{
				zlog_info(zlog_handler," ---------------- EVENT : MSG_DEL_DISCONNECT_USER: msg_number = %d",getData->msg_number);

				g_receive_para* tmp_receive = (g_receive_para*)getData->tmp_data;

				unregisterEvent(tmp_receive->connfd,g_server);
				
				del_user(tmp_receive->connfd, g_server, g_broker, g_dma, g_threadpool);

				display(g_server);

				break;
			}
			/* process openwrt msg */
			case MSG_OPENWRT_CONNECTED:
			{
				zlog_info(zlog_handler," ---------------- EVENT : MSG_OPENWRT_CONNECTED: msg_number = %d",getData->msg_number);
			
				if(g_server->openwrt_link == 0){
					break;
				}

				user_session_node* new_user = new_user_node(g_server);
            	int ret = CreateRecvParam(new_user->g_receive, g_server->g_msg_queue, g_server->openwrt_connfd, g_server->log_handler);
				add_new_user_node_to_list(new_user, g_server);
				// openwrt_connfd add to epoll? --- 0527
				// 用pipe 或 eventfd 是常规的做法,我见过的网络库都这么做
				// may be lose receive info  --- 0527

				uint64_t count; // event_fd need 8 byte !!!
				memcpy(&count,&new_user,sizeof(void*));
        		ret = write(g_server->epoll_node.event_fd, &count, sizeof(count));
				if(ret == -1){
					del_user(g_server->openwrt_connfd, g_server, g_broker, g_dma, g_threadpool); // fail to add new fd to epoll !!
				}
				if(g_broker->enableCallback == 0){
					zlog_info(zlog_handler," ---------------- EVENT : MSG_OPENWRT_CONNECTED: register broker callback, openwrt_connfd = %d \n", g_server->openwrt_connfd);
					broker_register_callback_interface(g_broker);
					g_broker->enableCallback = 1; 
				}
				
				display(g_server);

				break;
			}
			case MSG_OPENWRT_DISCONNECT:
			{
				zlog_info(zlog_handler," ---------------- EVENT : MSG_OPENWRT_DISCONNECT: msg_number = %d",getData->msg_number);

				// add clear recevie param  --- 20200520
				g_receive_para* g_receive = findReceiveNode(g_server->openwrt_connfd, g_server);

				postMsg(MSG_DEL_DISCONNECT_USER,NULL,0,g_receive,0,g_receive->g_msg_queue);

				break;
			}
			/* ----- end process openwrt msg -------*/
			case MSG_INQUIRY_SYSTEM_STATE:
			{
				zlog_info(zlog_handler," ---------------- EVENT : MSG_INQUIRY_SYSTEM_STATE: msg_number = %d",getData->msg_number);
				tmp_web = (web_msg_t*)getData->tmp_data;
				g_receive_para* tmp_receive = (g_receive_para*)tmp_web->point_addr_1;		

				// add new json parse to distinguish different terminal and terminal system time
				inquiry_system_state(tmp_receive,g_broker);

				// update device system time at inital access
				if(g_server->update_system_time){
					struct timeval tv;
					gettimeofday(&tv, NULL);
					g_broker->update_acc_time = tv.tv_sec - g_broker->start_time;

					if(tmp_web->buf_data_len != 0){
						changeSystemTime(tmp_web->currentTime);
					}else{
						changeSystemTime("2000-03-06 14:40:00");
					}
					g_server->update_system_time = 0;
					gettimeofday(&tv, NULL);
					g_broker->start_time = tv.tv_sec;
					zlog_info(zlog_handler,"update device system time \n");
				}

				struct user_session_node *pnode = NULL;
				pnode = find_user_node_by_connfd(tmp_receive->connfd, g_server);
				if(pnode != NULL){
					if(pnode->user_ip == NULL){
						pnode->user_ip = malloc(64);
						memcpy(pnode->user_ip,tmp_web->localIP,strlen(tmp_web->localIP)+1);
					}
				}

				zlog_info(zlog_handler,"localIp = %s \n", tmp_web->localIP);

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

				//monitorManageInfo(g_server, g_broker, g_dma);

				addTimeOutWorkToTimer(g_msg_queue,g_timer);

				break;
			}
			case MSG_TIMEOUT_TEST:
			{
				
				zlog_info(zlog_handler," ---------------- EVENT : MSG_TIMEOUT_TEST: msg_number = %d",getData->msg_number);

				// struct LogTaskTimer_t* tmp = (struct LogTaskTimer_t*)(getData->tmp_data);

				// int tmp_num = *(tmp->cnt_num);
				// zlog_info(zlog_handler,"*****************  num = %d \n",tmp_num);

				// addLogTaskToTimer(g_msg_queue, tmp->cnt_num, g_timer);

				// free(tmp);

				break;
			}
			case MSG_INQUIRY_REG_STATE:
			{
				tmp_web = (web_msg_t*)getData->tmp_data;
				g_receive_para* tmp_receive = (g_receive_para*)tmp_web->point_addr_1;				

				inquiry_reg_state(tmp_receive, g_broker);
				
				break;
			}
			case MSG_INQUIRY_RSSI: // open rssi
			{
				zlog_info(zlog_handler," ---------------- EVENT : MSG_INQUIRY_RSSI: msg_number = %d",getData->msg_number);
				
				tmp_web = (web_msg_t*)getData->tmp_data;
				g_receive_para* tmp_receive = (g_receive_para*)tmp_web->point_addr_1;				
				
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
				
				tmp_web = (web_msg_t*)getData->tmp_data;
				g_receive_para* tmp_receive = (g_receive_para*)tmp_web->point_addr_1;				

				int ret = process_rssi_save_file(tmp_receive->connfd, getData->msg_json,getData->msg_len,g_broker);
				/* record rssi save cmd : if check return of process_rssi_save_file() ?*/
				if(ret == 0){
					record_rssi_save_enable(tmp_receive->connfd, getData->msg_json, getData->msg_len, g_server);
				}
				/* inform node js cmd state */
				send_cmd_state(g_server, tmp_receive ,ret, g_record->constrol_rssi_succ);
				break;
			}
			case MSG_CLEAR_RSSI_WRITE_STATUS: // case 2 : if not close save rssi manually, this event must be behind MSG_RECEIVE_THREAD_CLOSED
			{
				zlog_info(zlog_handler," ---------------- EVENT : MSG_CLEAR_RSSI_WRITE_STATUS: msg_number = %d",getData->msg_number);

				rssi_user_node* tmp_node = (rssi_user_node*)getData->tmp_data;

				clear_rssi_write_status(tmp_node,g_broker);

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

				int state_cnt = -1;
				struct user_session_node *pnode = NULL;
				list_for_each_entry(pnode, &g_server->user_session_node_head, list) {
					if(pnode->g_receive != NULL){
						if(state_id == 11){
							;
						}else if(state_id == 22){
							;
						}
						break;			
					}
				}

				cJSON_Delete(root);
				free(p_conf_file);

				break;
			}
			case MSG_START_CSI:
			{
				zlog_info(zlog_handler," ---------------- EVENT : MSG_START_CSI: msg_number = %d",getData->msg_number);
				
				tmp_web = (web_msg_t*)getData->tmp_data;
				g_receive_para* tmp_receive = (g_receive_para*)tmp_web->point_addr_1;				
				
				/* check mutex with constell */
				int state = check_constell_working(g_server);
				if(state == 1){
					send_cmd_state(g_server,tmp_receive ,CSI_MUTEX, g_record->start_csi_mutex);
					break;
				}

				state = start_csi_state_external(tmp_receive->connfd,g_dma);

				if(state == 0){
					record_csi_start_enable(tmp_receive->connfd, START, g_server);
				}

				/* inform node js cmd state */
				send_cmd_state(g_server,tmp_receive ,state, g_record->start_csi_succ);


				break;
			}
			case MSG_STOP_CSI:
			{
				zlog_info(zlog_handler," ---------------- EVENT : MSG_STOP_CSI: msg_number = %d",getData->msg_number);

				tmp_web = (web_msg_t*)getData->tmp_data;
				g_receive_para* tmp_receive = (g_receive_para*)tmp_web->point_addr_1;				

				stop_csi_state_external(tmp_receive->connfd, g_dma);

				record_csi_start_enable(tmp_receive->connfd, STOP, g_server);

				/* inform node js cmd state */
				send_cmd_state(g_server,tmp_receive,CMD_OK,g_record->stop_csi_succ);

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

				tmp_web = (web_msg_t*)getData->tmp_data;
				g_receive_para* tmp_receive = (g_receive_para*)tmp_web->point_addr_1;				

				/* check mutex with csi */
				int state = check_csi_working(g_server);
				if(state == 1){
					send_cmd_state(g_server,tmp_receive ,CONSTELL_MUTEX, g_record->start_constell_mutex);
					break;
				}

				start_constellation_external(tmp_receive->connfd,g_dma);

				record_constell_start_enable(tmp_receive->connfd, START, g_server);

				send_cmd_state(g_server,tmp_receive ,CMD_OK, g_record->start_constell_succ);

				break;
			}
			case MSG_STOP_CONSTELLATION:
			{
				zlog_info(zlog_handler," ---------------- EVENT : MSG_STOP_CONSTELLATION: msg_number = %d",getData->msg_number);

				tmp_web = (web_msg_t*)getData->tmp_data;
				g_receive_para* tmp_receive = (g_receive_para*)tmp_web->point_addr_1;				

				stop_constellation_external(tmp_receive->connfd, g_dma);

				record_constell_start_enable(tmp_receive->connfd, STOP, g_server);

				send_cmd_state(g_server,tmp_receive ,CMD_OK, g_record->stop_csi_succ);

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
				
				tmp_web = (web_msg_t*)getData->tmp_data;
				g_receive_para* tmp_receive = (g_receive_para*)tmp_web->point_addr_1;			

				process_csi_save_file(tmp_receive->connfd, getData->msg_json,getData->msg_len, g_dma);

				record_csi_save_enable(tmp_receive->connfd, getData->msg_json, getData->msg_len, g_server);

				send_cmd_state(g_server,tmp_receive ,CMD_OK, g_record->csi_save_succ);

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
				
				tmp_web = (web_msg_t*)getData->tmp_data;
				g_receive_para* tmp_receive = (g_receive_para*)tmp_web->point_addr_1;				

				system("sh /tmp/gw_app/distance/conf/open_app.sh");
				send_cmd_state(g_server,tmp_receive ,CMD_OK, g_record->open_distance_succ);
				break;
			}
			case MSG_CLOSE_DISTANCE_APP:
			{
				zlog_info(zlog_handler," ---------------- EVENT : MSG_CLOSE_DISTANCE_APP: msg_number = %d",getData->msg_number);

				tmp_web = (web_msg_t*)getData->tmp_data;
				g_receive_para* tmp_receive = (g_receive_para*)tmp_web->point_addr_1;				

				system("sh /tmp/gw_app/distance/conf/close_app.sh");
				send_cmd_state(g_server,tmp_receive ,CMD_OK,g_record->close_distance_succ);
				break;
			}
			case MSG_OPEN_DAC:
			{
				zlog_info(zlog_handler," ---------------- EVENT : MSG_OPEN_DAC: msg_number = %d",getData->msg_number);

				tmp_web = (web_msg_t*)getData->tmp_data;
				g_receive_para* tmp_receive = (g_receive_para*)tmp_web->point_addr_1;				

				system("echo 1 > /sys/class/gpio/gpio973/value");
				send_cmd_state(g_server,tmp_receive ,CMD_OK, g_record->open_dac_succ);
				break;
			}
			case MSG_CLOSE_DAC:
			{
				zlog_info(zlog_handler," ---------------- EVENT : MSG_CLOSE_DAC: msg_number = %d",getData->msg_number);

				tmp_web = (web_msg_t*)getData->tmp_data;
				g_receive_para* tmp_receive = (g_receive_para*)tmp_web->point_addr_1;			

				system("echo 0 > /sys/class/gpio/gpio973/value");
				send_cmd_state(g_server,tmp_receive ,CMD_OK, g_record->close_dac_succ);
				break;
			}
			case MSG_CLEAR_LOG:
			{
				zlog_info(zlog_handler," ---------------- EVENT : MSG_CLEAR_LOG: msg_number = %d",getData->msg_number);

				tmp_web = (web_msg_t*)getData->tmp_data;
				g_receive_para* tmp_receive = (g_receive_para*)tmp_web->point_addr_1;			

				system("sh /tmp/web/conf/clear_log.sh");
				send_cmd_state(g_server,tmp_receive ,CMD_OK, g_record->clear_log_succ);
				break;
			}
			case MSG_RESET_SYSTEM:
			{
				tmp_web = (web_msg_t*)getData->tmp_data;
				g_receive_para* tmp_receive = (g_receive_para*)tmp_web->point_addr_1;	

				send_cmd_state(g_server,tmp_receive ,CMD_OK, g_record->reset_sys_succ);
				system("reboot");
				break;
			}
			case MSG_INQUIRY_STATISTICS:
			{
				tmp_web = (web_msg_t*)getData->tmp_data;
				g_receive_para* tmp_receive = (g_receive_para*)tmp_web->point_addr_1;		

				inquiry_statistics(tmp_receive, g_broker);
				break;
			}
			case MSG_IP_SETTING:
			{
				zlog_info(zlog_handler," ---------------- EVENT : MSG_IP_SETTING: msg_number = %d",getData->msg_number);
				
				tmp_web = (web_msg_t*)getData->tmp_data;
				g_receive_para* tmp_receive = (g_receive_para*)tmp_web->point_addr_1;		

				int ret = process_ip_setting(getData->msg_json, getData->msg_len,g_broker->log_handler);
				send_cmd_state(g_server,tmp_receive ,CMD_OK,g_record->ip_setting_succ);
				break;
			}
			case MSG_INQUIRY_RF_INFO:
			{
				tmp_web = (web_msg_t*)getData->tmp_data;
				g_receive_para* tmp_receive = (g_receive_para*)tmp_web->point_addr_1;		

				int user_node_id = find_user_node_id(tmp_receive->connfd, g_server);
				//zlog_info(zlog_handler," ---------------- EVENT : MSG_INQUIRY_RF_INFO: start time user_id = %d", user_node_id);
				postRfWorkToThreadPool(user_node_id, g_broker, g_threadpool);
				break;
			}
			case MST_RF_INFO_READY: // bug : 0302 --- MST_RF_INFO_READY may after MSG_RECEIVE_THREAD_CLOSED
			{
				tmp_web = (web_msg_t*)getData->tmp_data;
				int tmp_node_id = tmp_web->arg_1;
				char* response_json = tmp_web->buf_data;	

				//zlog_info(g_broker->log_handler,"rf_info_response : %s \n", response_json);
				struct user_session_node* tmp_node = find_user_node_by_user_id(tmp_node_id, g_server);
				if(tmp_node != NULL){
					int ret = assemble_frame_and_send(tmp_node->g_receive,response_json,strlen(response_json),TYPE_RF_INFO_RESPONSE);
				}
				free(response_json);
				//zlog_info(zlog_handler," ********************* EVENT : MST_RF_INFO_READY: End Time user_id = %d", tmp_node_id);
				break;
			}
			case MSG_RF_FREQ_SETTING:
			{
				zlog_info(zlog_handler," ---------------- EVENT : MSG_RF_FREQ_SETTING: msg_number = %d",getData->msg_number);
				tmp_web = (web_msg_t*)getData->tmp_data;
				g_receive_para* tmp_receive = (g_receive_para*)tmp_web->point_addr_1;	

				int ret = process_rf_freq_setting(getData->msg_json, getData->msg_len,g_broker);
				break;
			}
			case MSG_OPEN_TX_POWER:
			{
				zlog_info(zlog_handler," ---------------- EVENT : MSG_OPEN_TX_POWER: msg_number = %d",getData->msg_number);
				tmp_web = (web_msg_t*)getData->tmp_data;
				g_receive_para* tmp_receive = (g_receive_para*)tmp_web->point_addr_1;	

				int ret = open_tx_power();

				zlog_info(zlog_handler, "TX _ POWER : %s \n", g_record->open_txpower_succ);

				send_cmd_state(g_server,tmp_receive ,CMD_OK, g_record->open_txpower_succ);
				break;
			}
			case MSG_CLOSE_TX_POWER:
			{
				zlog_info(zlog_handler," ---------------- EVENT : MSG_CLOSE_TX_POWER: msg_number = %d",getData->msg_number);
				tmp_web = (web_msg_t*)getData->tmp_data;
				g_receive_para* tmp_receive = (g_receive_para*)tmp_web->point_addr_1;

				int ret = close_tx_power();
				send_cmd_state(g_server,tmp_receive ,CMD_OK, g_record->close_txpower_succ);
				break;
			}
			case MSG_OPEN_RX_GAIN:
			{
				zlog_info(zlog_handler," ---------------- EVENT : MSG_OPEN_RX_GAIN: msg_number = %d",getData->msg_number);
				tmp_web = (web_msg_t*)getData->tmp_data;
				g_receive_para* tmp_receive = (g_receive_para*)tmp_web->point_addr_1;				
				
				int ret = rx_gain_high();
				send_cmd_state(g_server,tmp_receive ,CMD_OK, g_record->open_rxgain_succ);
				break;
			}
			case MSG_CLOSE_RX_GAIN:
			{
				zlog_info(zlog_handler," ---------------- EVENT : MSG_CLOSE_RX_GAIN: msg_number = %d",getData->msg_number);
				tmp_web = (web_msg_t*)getData->tmp_data;
				g_receive_para* tmp_receive = (g_receive_para*)tmp_web->point_addr_1;				

				int ret = rx_gain_normal();
				send_cmd_state(g_server,tmp_receive ,CMD_OK, g_record->close_rxgain_succ);
				break;
			}
			default:
				break;
		}// end switch
		if(tmp_web != NULL){
			free(tmp_web);
		}
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
	free(tmp_node->user_ip);
	free(tmp_node);
}

/* ------------------------------ rssi record function ---------------------------- */

void record_rssi_enable(int connfd, g_server_para* g_server){

	struct user_session_node *pnode = NULL;
	pnode = find_user_node_by_connfd(connfd, g_server);
	if(pnode != NULL){
		pnode->record_action->enable_rssi = 1;
		zlog_info(g_server->log_handler,"connfd = %d , enable_rssi = 1 \n", connfd);
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
	pnode = find_user_node_by_connfd(connfd, g_server);
	if(pnode != NULL){
		pnode->record_action->enable_rssi_save = rssi_save;
		zlog_info(g_server->log_handler,"connfd = %d , enable_rssi_save = %d \n", connfd, rssi_save);
	}

	cJSON_Delete(root);
	return 0;
}

/* --------------------------------- csi record function ------------------------------------------ */
void record_csi_start_enable(int connfd, int enable, g_server_para* g_server){
	struct user_session_node *pnode = NULL;
	pnode = find_user_node_by_connfd(connfd, g_server);
	if(pnode != NULL){
		pnode->record_action->enable_start_csi = enable;
		zlog_info(g_server->log_handler,"connfd = %d , enable_start_csi = %d \n", connfd, enable);
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
	pnode = find_user_node_by_connfd(connfd, g_server);
	if(pnode != NULL){
		pnode->record_action->enable_csi_save = csi_save;
		zlog_info(g_server->log_handler,"connfd = %d , enable_csi_save = %d \n", connfd, csi_save);
	}

	cJSON_Delete(root);
	return 0;
}

/* --------------------------------- constellation record function ------------------------------------------ */
void record_constell_start_enable(int connfd, int enable, g_server_para* g_server){

	struct user_session_node *pnode = NULL;
	pnode = find_user_node_by_connfd(connfd, g_server);
	if(pnode != NULL){
		pnode->record_action->enable_start_constell = enable;
		zlog_info(g_server->log_handler,"connfd = %d , enable_start_constell = %d \n", connfd, enable);
	}
}


/* --------------------------  cmd CMD_OK or CMD_FAIL and mutex check between csi and constellation ----------------------------------*/
#define CMD_OK 0
#define CMD_FAIL -1
#define CSI_MUTEX 2
#define CONSTELL_MUTEX 3
void send_cmd_state(g_server_para* g_server, g_receive_para* g_receive ,int state, char* record_str){
	int cmd_state = CMD_FAIL;
	if(state == CMD_OK){
		cmd_state = CMD_OK;
	}else if(state == CSI_MUTEX){
		cmd_state = CSI_MUTEX;
	}else if(state == CONSTELL_MUTEX){
		cmd_state = CONSTELL_MUTEX;
	}
	
	char tmp_record[256];
	struct user_session_node *pnode = NULL;
	pnode = find_user_node_by_connfd(g_receive->connfd, g_server);
	if(pnode != NULL){
		sprintf(tmp_record, "%s : %s", pnode->user_ip, record_str);
	}else{
		sprintf(tmp_record, "No user : %s", record_str);
	}
	char *cmd_state_response_json = cmd_state_response(cmd_state,tmp_record);
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

