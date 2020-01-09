#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "broker.h"
#include "mosquitto_broker.h"
#include "cJSON.h"
#include "response_json.h"
#include "small_utility.h"
#include "auto_log.h"

g_broker_para* g_broker_temp = NULL;

/* ----------------------- common use ------------------------- */
int inquiry_dac_state(){
	int value = gpio_read(973);
	return value;
}

g_receive_para* findReceiveNode(int connfd, g_broker_para* g_broker){
	struct user_session_node *pnode = NULL;
	g_receive_para* tmp_receive = NULL;
	list_for_each_entry(pnode, &g_broker->g_server->user_session_node_head, list) {
		if(pnode->g_receive->connfd == connfd){
			tmp_receive = pnode->g_receive;
			break;
		}
	}
	return tmp_receive;
}

rssi_user_node* findNode(int connfd, g_broker_para* g_broker){
	struct rssi_user_node *pnode = NULL;
	struct rssi_user_node *tmp = NULL;
	list_for_each_entry(tmp, &g_broker->rssi_user_node_head, list) {
		if(tmp->connfd == connfd){
			pnode = tmp;    
			break;
		}
	}
	return pnode;
}

system_state_t* get_system_state(g_broker_para* g_broker, char* system_str, int ready){
	system_state_t* tmp = (system_state_t*)malloc(sizeof(system_state_t));
	if(system_str != NULL){
		memcpy(tmp->system_str,system_str,strlen(system_str)+1);
	}
	if(ready == 1){
		tmp->soft_version = c_compiler_builtin_macro();
		uint32_t value = get_fpga_version(g_broker->g_RegDev);
		tmp->fpga_version = parse_fpga_version(value);
		tmp->dac_state = inquiry_dac_state();
		tmp->distance_state = IsProcessIsRun("distance_main");
		tmp->frequency = 77575;
		tmp->tx_power_state = 1;
		tmp->rx_gain_state = 0;
	}
	return tmp;
}

void clear_system_state(system_state_t* tmp){
	if(tmp->fpga_version != NULL){
		free(tmp->fpga_version);
	}
	if(tmp->soft_version != NULL){
		free(tmp->soft_version);
	}
	free(tmp);
}

/* ----------------------- auto log -------------------------------- */
void my_log_data(double rssi_data, g_broker_para* g_broker){
	/* snr */
	double snr = get_rx_snr(g_broker->g_RegDev);
	/* distance */
	uint32_t delay_arm_write = get_delay_RW(g_broker->g_RegDev);
	uint32_t value = get_delay_tick(g_broker->g_RegDev);
	double distance = (value - delay_arm_write) * 0.149896229;

	log_data_t* data = (log_data_t*)malloc(sizeof(log_data_t));
	data->rssi = rssi_data;
	data->snr  = snr;
	data->distance = distance;

	auto_save_data_log(&logc, data, LOG_DATA);
}	

/* ----------------------- common use ------------------------- */


void inquiry_system_json(g_broker_para* g_broker){
    cJSON *root = cJSON_CreateObject();
	cJSON_AddStringToObject(root, "stat", "0");
	cJSON_AddStringToObject(root, "dst", "mon");
    g_broker->json_set.system_state_json = cJSON_Print(root);
    cJSON_Delete(root);
}

int createBroker(char *argv, g_broker_para** g_broker, g_server_para* g_server, g_RegDev_para* g_RegDev, zlog_category_t* handler){

	zlog_info(handler,"createBroker()");
	*g_broker = (g_broker_para*)malloc(sizeof(struct g_broker_para));
	(*g_broker)->g_server      	   = g_server;                                    // connect_fd : g_broker->g_server->g_receive->connfd
	(*g_broker)->g_msg_queue       = g_server->g_msg_queue;
	(*g_broker)->log_handler       = handler;
    (*g_broker)->system_ready      = 0;
    (*g_broker)->g_RegDev          = g_RegDev;
	(*g_broker)->enableCallback    = 0;
	(*g_broker)->start_time        = now();

	INIT_LIST_HEAD(&((*g_broker)->rssi_user_node_head));
	(*g_broker)->rssi_module.rssi_state = 0;
	(*g_broker)->rssi_module.user_cnt   = 0;
	

	g_broker_para* tmp_broker = *g_broker;

    char* rssi_open_path = "../conf/rssi_open.json";
    (*g_broker)->json_set.rssi_open_json = readfile(rssi_open_path);

    char* rssi_close_path = "../conf/rssi_close.json";
    (*g_broker)->json_set.rssi_close_json = readfile(rssi_close_path);

    inquiry_system_json(*g_broker);

	int ret = init_broker(get_prog_name(argv), NULL, -1, NULL, NULL);
	zlog_info(handler,"get_prog_name(argv) = %s , ret = %d \n",get_prog_name(argv),ret);
	if( ret != 0)
		return -2;

	g_broker_temp = *g_broker;
	zlog_info(handler,"end createBroker()\n");
	return 0;
}

/* other thread call */
int inform_exception(char* buf, int buf_len, char *from, void* arg)
{ 
	int ret = 0;
	if(strcmp(from,"mon/all/pub/system_stat") == 0){
		zlog_info(g_broker_temp->log_handler,"inform_exception: mon/all/pub/system_stat , buf = %s \n", buf);
        postMsg(MSG_SYSTEM_STATE_EXCEPTION, buf, buf_len, NULL, 0, g_broker_temp->g_msg_queue);	
	}else if(strcmp(from,"rf/all/pub/rssi") == 0){
		postMsg(MSG_RSSI_READY_AND_SEND, buf, buf_len, NULL, 0, g_broker_temp->g_msg_queue);	
	}
	return ret;
}

void process_exception(char* stat_buf, int stat_buf_len, g_broker_para* g_broker){
	cJSON * root = NULL;
    cJSON * item = NULL;
    root = cJSON_Parse(stat_buf);
    item = cJSON_GetObjectItem(root,"stat");
	int is_exception = 1;
	if(strcmp(item->valuestring,"0x20") == 0 || strcmp(item->valuestring,"0x80000020") == 0){
		if(g_broker->system_ready == 0){
			g_broker->system_ready = 1;
			system_state_t* state = get_system_state(g_broker,NULL,g_broker->system_ready);
			char* response_json = system_state_response(g_broker->system_ready,is_exception,state);

			struct user_session_node *pnode = NULL;
			list_for_each_entry(pnode, &g_broker->g_server->user_session_node_head, list) {
				if(pnode->g_receive != NULL){    
					assemble_frame_and_send(pnode->g_receive,response_json,strlen(response_json),TYPE_SYSTEM_STATE_EXCEPTION);
				}
			}

            free(response_json);
			clear_system_state(state);
		}else{
            ; // do not inform node.js
        }			
	}else{
		g_broker->system_ready = 0;
		char tmp_str[256];
		sprintf(tmp_str, "device is not ready ! %s", item->valuestring);
		zlog_info(g_broker->log_handler,"exception other msg : %s ", tmp_str);

		system_state_t* state = get_system_state(g_broker,tmp_str,g_broker->system_ready);
		char* response_json = system_state_response(g_broker->system_ready,is_exception,state);

		struct user_session_node *pnode = NULL;
		list_for_each_entry(pnode, &g_broker->g_server->user_session_node_head, list) {
			if(pnode->g_receive != NULL){    
				assemble_frame_and_send(pnode->g_receive,response_json,strlen(response_json),TYPE_SYSTEM_STATE_EXCEPTION);
			}
		}
		zlog_info(g_broker->log_handler,"response_json: %s \n length = %d ", response_json, strlen(response_json));
		free(response_json);
		/* clear and reset system state variable */
		/* ???????????? */
		auto_save_alarm_log(&logc, tmp_str, strlen(tmp_str)+1, LOG_RADIO_EXCEPTION);
		clear_system_state(state);
	}
	cJSON_Delete(root);
}

int broker_register_callback_interface(g_broker_para* g_broker){
	int ret = register_callback("all", inform_exception, "#");
	if(ret != 0){
		zlog_error(g_broker->log_handler,"register_callback error in initBroker\n");
		return -1;
	}
	return 0;
}

void destoryBroker(g_broker_para* g_broker){
	close_broker();
	free(g_broker);
}

void parse_system_state(g_receive_para* tmp_receive, char* stat_buf, int stat_buf_len, g_broker_para* g_broker){
	cJSON * root = NULL;
    cJSON * item = NULL;
    root = cJSON_Parse(stat_buf);
    item = cJSON_GetObjectItem(root,"stat");
	int is_exception = 0;
	if(strcmp(item->valuestring,"0x20") == 0 || strcmp(item->valuestring,"0x80000020") == 0){
		g_broker->system_ready = 1;
		// send frame to node js
		system_state_t* state = get_system_state(g_broker,NULL,g_broker->system_ready);
		char* response_json = system_state_response(g_broker->system_ready,is_exception,state);
		assemble_frame_and_send(tmp_receive,response_json,strlen(response_json),TYPE_SYSTEM_STATE_RESPONSE);
		free(response_json);
		clear_system_state(state);		
	}else{
		g_broker->system_ready = 0;
		char tmp_str[256];
		sprintf(tmp_str, "device is not ready , SystemState not 0x20 !!! %s \n", item->valuestring);
		zlog_info(g_broker->log_handler,"other msg : %s ", tmp_str);
		system_state_t* state = get_system_state(g_broker,tmp_str,g_broker->system_ready);
		char* response_json = system_state_response(g_broker->system_ready,is_exception,state);
		assemble_frame_and_send(tmp_receive,response_json,strlen(response_json),TYPE_SYSTEM_STATE_RESPONSE);
		free(response_json);
		clear_system_state(state);	
	}
	cJSON_Delete(root);
    free(stat_buf);
}

int inquiry_system_state(g_receive_para* tmp_receive, g_broker_para* g_broker){
	int ret = -1;
	char* stat_buf = NULL;
	int stat_buf_len = 0;

    char *buf = g_broker->json_set.system_state_json;
    int buf_len = strlen(buf)+1;

	cJSON * root = NULL;
    cJSON * item = NULL;
	cJSON * item_type = NULL;
    root = cJSON_Parse(buf);
    item = cJSON_GetObjectItem(root,"dst");
	ret = dev_transfer(buf, buf_len, &stat_buf, &stat_buf_len, "mon", -1);
	if(ret == 0 && stat_buf_len > 0){
		// system monitor State
        parse_system_state(tmp_receive,stat_buf,stat_buf_len,g_broker);
	}else{
		zlog_info(g_broker->log_handler,"no data in mosquitto \n ");
		// need return if no data in mosquitto
	}
	return ret;
}

int inquiry_reg_state(g_receive_para* tmp_receive, g_broker_para* g_broker){
	reg_state_t* reg_state = (reg_state_t*)malloc(sizeof(reg_state_t));
	reg_state->reg_state_num = 0;
	/* power_est_latch */
	reg_state->power_est_latch = getPowerLatch(g_broker->g_RegDev);
	reg_state->reg_state_num++;
	/* sync_failed_stastic */
	uint32_t value = get_sync_failed_stastic(g_broker->g_RegDev);
	reg_state->coarse_low16	= (value << 16) >> 16;
	reg_state->reg_state_num++;
	reg_state->fine_high16	= value >> 16;
	reg_state->reg_state_num++;
	/* freq_offset */
	value = get_freq_offset(g_broker->g_RegDev);
	reg_state->freq_offset  = calculateFreq(value);
	reg_state->reg_state_num++;
	/* pac_txc_misc */
	value = get_pac_txc_misc(g_broker->g_RegDev);
	unsigned int mcs_pos = 1015808;
	reg_state->tx_modulation = (value & mcs_pos) >> 15;
	reg_state->reg_state_num++;
	/* pac_txc_re_trans_cnt */
	reg_state->txc_retrans_cnt = get_pac_txc_re_trans_cnt(g_broker->g_RegDev);
	reg_state->reg_state_num++;
	/* pac_txc_expect_seq_id */
	value = get_pac_txc_expect_seq_id(g_broker->g_RegDev);
	reg_state->expect_seq_id_high16 = value >> 16;
	reg_state->reg_state_num++;
	reg_state->expect_seq_id_low16 = (value << 16) >> 16;
	reg_state->reg_state_num++;
	/* rxc_miscs */
	value = get_rxc_miscs(g_broker->g_RegDev);
	reg_state->rx_id = (value << 16) >> 16;
	reg_state->reg_state_num++;
	reg_state->rx_fifo_data = (value << 10) >> 26;
	reg_state->reg_state_num++;
	/* rx_sync */
	reg_state->rx_sync = get_rx_sync(g_broker->g_RegDev);
	reg_state->reg_state_num++;
	/* ctrl_frame_crc_correct_cnt */
	reg_state->ctrl_crc_c = get_ctrl_frame_crc_correct_cnt(g_broker->g_RegDev);
	reg_state->reg_state_num++;
	/* ctrl_frame_crc_error_cnt */
	reg_state->ctrl_crc_e = get_ctrl_frame_crc_error_cnt(g_broker->g_RegDev);
	reg_state->reg_state_num++;
	/* manage_frame_crc_correct_cnt */
	reg_state->manage_crc_c = get_manage_frame_crc_correct_cnt(g_broker->g_RegDev);
	reg_state->reg_state_num++;
	/* manage_frame_crc_error_cnt */
	reg_state->manage_crc_e = get_manage_frame_crc_error_cnt(g_broker->g_RegDev);
	reg_state->reg_state_num++;
	/* bb_send_cnt */
	reg_state->bb_send_cnt = get_bb_send_cnt(g_broker->g_RegDev);
	reg_state->reg_state_num++;
	/* rx_vector */
	value = get_rx_vector(g_broker->g_RegDev);
	reg_state->rx_v_agg_num = (value << 5) >> 28;
	reg_state->reg_state_num++;
	reg_state->rx_v_len     = (value << 14) >> 14;
	reg_state->reg_state_num++;
	/* pac_soft_rst */
	value = get_pac_soft_rst(g_broker->g_RegDev);
	reg_state->tx_abort = (value << 24) >> 31;
	reg_state->reg_state_num++;
	reg_state->ddr_closed = (value << 29) >> 31;
	reg_state->reg_state_num++;
	/* sw_fifo_data_cnt */
	reg_state->sw_fifo_cnt = get_sw_fifo_data_cnt(g_broker->g_RegDev);
	reg_state->reg_state_num++;
	/* dac state */
	reg_state->dac_state = inquiry_dac_state();
	reg_state->reg_state_num++;
	/* snr */
	reg_state->snr = get_rx_snr(g_broker->g_RegDev);
	reg_state->reg_state_num++;
	/* distance */
	uint32_t delay_arm_write = get_delay_RW(g_broker->g_RegDev);
	value = get_delay_tick(g_broker->g_RegDev);
	reg_state->distance = (value - delay_arm_write) * 0.149896229;
	reg_state->reg_state_num++;

	char* reg_state_response_json = reg_state_response(reg_state);

	assemble_frame_and_send(tmp_receive,reg_state_response_json,strlen(reg_state_response_json),TYPE_REG_STATE_RESPONSE);

	free(reg_state);
	free(reg_state_response_json);

}

// ---------- rssi ----------------

int control_rssi_state(char *buf, int buf_len, g_broker_para* g_broker){
	zlog_info(g_broker->log_handler,"rssi json = %s \n",buf);
	int ret = -1;
	char* stat_buf = NULL;
	int stat_buf_len = 0;

	cJSON * root = NULL;
    cJSON * item = NULL;
    root = cJSON_Parse(buf);
    item = cJSON_GetObjectItem(root,"dst");

	ret = dev_transfer(buf, buf_len, &stat_buf, &stat_buf_len, item->valuestring, -1);

	if(ret == 0 && stat_buf_len > 0){
		item = cJSON_GetObjectItem(root,"timer");
		if(strcmp(item->valuestring,"0") == 0)
			g_broker->rssi_module.rssi_state = 0;
		else
			g_broker->rssi_module.rssi_state = 1;
		zlog_info(g_broker->log_handler,"rssi_state = %d \n, rssi return json = %s \n", g_broker->rssi_module.rssi_state, stat_buf);

		cJSON *ret_root = cJSON_Parse(stat_buf);
		cJSON *ret_item = cJSON_GetObjectItem(ret_root,"stat");
		if( strcmp(ret_item->valuestring,"0") == 0){
			ret = 0;
		}else{
			ret = -1;
		}
		free(stat_buf);
		cJSON_Delete(ret_root);
	}else{
		ret = -1;// if json process return error, postException msg and some rssi_enable state must change
	}
	cJSON_Delete(root);
	return ret;
}

int open_rssi_state_external(int connfd, g_broker_para* g_broker){
	int ret = -1;
	if(g_broker->rssi_module.user_cnt == 0 && g_broker->rssi_module.rssi_state == 0){
		zlog_info(g_broker->log_handler,"open rssi in control_rssi_state() \n");
		ret = control_rssi_state(g_broker->json_set.rssi_open_json,strlen(g_broker->json_set.rssi_open_json), g_broker);
		if(ret != 0){
			return -1;
		}
	}

    rssi_user_node* new_node = (rssi_user_node*)malloc(sizeof(rssi_user_node));
	new_node->g_broker = g_broker;
    new_node->connfd = connfd;
    new_node->rssi_file_t = NULL;
	new_node->confirm_delete = 0;

    list_add_tail(&new_node->list, &g_broker->rssi_user_node_head);
	g_broker->rssi_module.user_cnt++;
	zlog_info(g_broker->log_handler,"open rssi user_cnt = %d \n",g_broker->rssi_module.user_cnt);
	return 0;
}

/* call only by close webpage */
int close_rssi_state_external(int connfd, g_broker_para* g_broker){
    struct list_head *pos, *n;
    struct rssi_user_node *pnode = NULL;
    list_for_each_safe(pos, n, &g_broker->rssi_user_node_head){
        pnode = list_entry(pos, struct rssi_user_node, list);
        if(pnode->connfd == connfd){
            list_del(pos);
            g_broker->rssi_module.user_cnt--;
			zlog_info(g_broker->log_handler,"find node in close_rssi_state_external() ! connfd = %d, user_cnt = %d ", connfd,g_broker->rssi_module.user_cnt);
            break;
        }
    }

	if(pnode != NULL){
		// free pnode
		assert(pnode->confirm_delete == 0);
		if(pnode->rssi_file_t == NULL){
			zlog_info(g_broker->log_handler," close_rssi_state_external : delete from list , rssi_file = NULL , so free this node 0x%x \n", pnode);
			free(pnode);
		}else{
			zlog_error(g_broker->log_handler," close_rssi_state_external : delete from list , not free this node 0x%x \n", pnode);
			pnode->confirm_delete = 1;
		}		
	}

	if(g_broker->rssi_module.user_cnt == 0 && g_broker->rssi_module.rssi_state == 1){
		zlog_info(g_broker->log_handler,"close rssi in control_rssi_state() \n");
		control_rssi_state(g_broker->json_set.rssi_close_json,strlen(g_broker->json_set.rssi_close_json), g_broker);
	}

	return 0;
}

void send_rssi_in_event_loop(char* buf, int buf_len, g_broker_para* g_broker){
	struct rssi_priv* tmp_buf = (struct rssi_priv*)buf;
	char tmp_c = *(tmp_buf->rssi_buf);
	int tmp = (int)tmp_c;
	tmp = tmp * 4;
	if(tmp < 0){
		tmp = tmp + 1024;
	}
	double rssi_data = (tmp * 5.0) / 1024;
	rssi_data = (rssi_data - 0.05) / 0.05 - 69.0;

	char* rssi_data_response_json = rssi_data_response(rssi_data);

	struct rssi_user_node *pnode = NULL;
	list_for_each_entry(pnode, &g_broker->rssi_user_node_head, list) {
		g_receive_para* tmp_receive = findReceiveNode(pnode->connfd,g_broker);
		if(tmp_receive != NULL)
			assemble_frame_and_send(tmp_receive,rssi_data_response_json,strlen(rssi_data_response_json),TYPE_RSSI_DATA_RESPONSE);
	}

	free(rssi_data_response_json);

	my_log_data(rssi_data,g_broker);
}

/* ????? */
void* rssi_write_thread(void* args){

	pthread_detach(pthread_self());

	rssi_user_node* tmp_node = (rssi_user_node*)args;

	g_broker_para* g_broker = tmp_node->g_broker;

	zlog_info(g_broker->log_handler,"start rssi_write_thread()\n");

	while(1){
		queue_item *work_item = tiny_queue_pop(tmp_node->rssi_file_t->queue); // need work length

		if(work_item->buf_len == 0){
			free(work_item);
			break;
		}

		char* work = work_item->buf;
		int32_t timestamp_tv_sec = *((int32_t*)work);
		int32_t timestamp_tv_usec = *((int32_t*)(work+ sizeof(int32_t) ));
		int32_t rssi_buf_len = *((int32_t*)(work+ sizeof(int32_t) * 2));
		printf("timestamp : sec = %d , usec = %d , rssi_len = %d \n", timestamp_tv_sec, timestamp_tv_usec, rssi_buf_len);
		fwrite(work,sizeof(char), work_item->buf_len, tmp_node->rssi_file_t->file);
		free(work);
		free(work_item);
	}

	postMsg(MSG_CLEAR_RSSI_WRITE_STATUS,NULL,0,tmp_node,0,g_broker->g_msg_queue);
	zlog_info(g_broker->log_handler,"end Exit rssi_write_thread()\n");
}

int process_rssi_save_file(int connfd, char* stat_buf, int stat_buf_len, g_broker_para* g_broker){
	cJSON * root = NULL;
    cJSON * item = NULL;
    root = cJSON_Parse(stat_buf);
    item = cJSON_GetObjectItem(root,"type");
	if(item->valueint != TYPE_RSSI_CONTROL){
		cJSON_Delete(root);
		return -1;
	}

	rssi_user_node* tmp_node = findNode(connfd, g_broker); // before save rssi , must be open rssi 
	if(tmp_node == NULL){
		zlog_error(g_broker->log_handler," No rssi user node in Node list ");
		return -2;
	}

	if(tmp_node->rssi_file_t == NULL){ // first access 
		tmp_node->rssi_file_t         = (write_file_t*)malloc(sizeof(write_file_t));
		pthread_mutex_init(&(tmp_node->rssi_file_t->mutex),NULL);
		tmp_node->rssi_file_t->enable = 0;
		tmp_node->rssi_file_t->file   = NULL;
		tmp_node->rssi_file_t->queue  = NULL;
	}

	item = cJSON_GetObjectItem(root,"op_cmd");
	if(item->valueint == 0){ /* stop save */
		if(tmp_node->rssi_file_t->enable == 0){
			zlog_error(g_broker->log_handler,"has already close rssi save in this page : %d \n", connfd);
			cJSON_Delete(root);
			return -1;
		}
		inform_stop_rssi_write_thread(connfd, g_broker);
	}else if(item->valueint == 1){ /* start save */
		if(tmp_node->rssi_file_t->enable == 1){
			zlog_error(g_broker->log_handler,"has already open rssi save in this page : %d \n", connfd);
			cJSON_Delete(root);
			return -1;
		}

		item = cJSON_GetObjectItem(root,"file_name");

		sprintf(tmp_node->rssi_file_t->file_name, "/run/media/mmcblk1p1/gw_web/web/log/%s", item->valuestring);
		printf("file_name : %s\n",tmp_node->rssi_file_t->file_name);
		tmp_node->rssi_file_t->file = fopen(tmp_node->rssi_file_t->file_name,"wb");
		if(tmp_node->rssi_file_t->file == NULL){
			zlog_error(g_broker->log_handler,"Cannot create the rssi file\n");
			cJSON_Delete(root);
			return -1;
		}

		tmp_node->rssi_file_t->queue  = tiny_queue_create();
		if (tmp_node->rssi_file_t->queue == NULL) {
			zlog_error(g_broker->log_handler,"Cannot create the rssi queue\n");
			cJSON_Delete(root);
			return -1;
		}

		pthread_t thread_pid;
		pthread_create(&thread_pid, NULL, rssi_write_thread, (void*)(tmp_node));
		tmp_node->rssi_file_t->enable = 1;
	}

	cJSON_Delete(root);
	return 0;
}

void send_rssi_to_save(char* buf, int buf_len, g_broker_para* g_broker){
	struct rssi_priv* tmp_buf = (struct rssi_priv*)buf;
	struct rssi_user_node *pnode = NULL;
	list_for_each_entry(pnode, &g_broker->rssi_user_node_head, list) {
		if(pnode->rssi_file_t != NULL){
			if(pnode->rssi_file_t->enable == 0){
				break;
			}else if(pnode->rssi_file_t->enable == 1){ /* for write file */
				int msg_len = 4 + 4 + 4 + tmp_buf->rssi_buf_len;
				char* rssi_buf = malloc(msg_len);
				*((int32_t*)rssi_buf) = tmp_buf->timestamp.tv_sec;
				*((int32_t*)(rssi_buf+ sizeof(int32_t) )) = tmp_buf->timestamp.tv_usec;
				*((int32_t*)(rssi_buf+ sizeof(int32_t) * 2)) = tmp_buf->rssi_buf_len;
				memcpy(rssi_buf + sizeof(int32_t) * 3, tmp_buf->rssi_buf, tmp_buf->rssi_buf_len);

				queue_item* item = (queue_item*)malloc(sizeof(queue_item));
				item->buf = rssi_buf;
				item->buf_len = msg_len;	

				if (tiny_queue_push(pnode->rssi_file_t->queue, item) != 0) {
					zlog_error(g_broker->log_handler,"Cannot push an element in the queue\n");
					free(rssi_buf);
					free(item);
				}

			}else if(pnode->rssi_file_t->enable == 2){
				zlog_error(g_broker->log_handler," pnode->rssi_file_t->enable == 2 in send_rssi_to_save()\n");
				queue_item* item = (queue_item*)malloc(sizeof(queue_item));
				item->buf = NULL;
				item->buf_len = 0;	

				if (tiny_queue_push(pnode->rssi_file_t->queue, item) != 0) {
					zlog_error(g_broker->log_handler,"Cannot push an 0 length element in the queue\n");
					free(item);
				}
				pnode->rssi_file_t->enable = 0;
			}
		}
	}
}

void clear_rssi_write_status(rssi_user_node* user_node, g_broker_para* g_broker){
	/* close file */
	fclose(user_node->rssi_file_t->file);
	user_node->rssi_file_t->file = NULL;

	/* destroy queue */
	tiny_queue_destory(user_node->rssi_file_t->queue);

	free(user_node->rssi_file_t);
	user_node->rssi_file_t = NULL;

	if(user_node->confirm_delete == 1){
		zlog_error(g_broker->log_handler," clear_rssi_write_status :  confirm free this node 0x%x \n", user_node);
		free(user_node);
	}else if(user_node->confirm_delete == 0){
		zlog_info(g_broker->log_handler," clear_rssi_write_status :  only clear rssi write component in this node 0x%x \n", user_node);
	}
}

void inform_stop_rssi_write_thread(int connfd, g_broker_para* g_broker){
	rssi_user_node* pnode = findNode(connfd, g_broker);
	if(pnode == NULL){
		zlog_error(g_broker->log_handler, "inform_stop_rssi_write_thread , No user node find");
		return;
	}

	queue_item* item = (queue_item*)malloc(sizeof(queue_item));
	item->buf = NULL;
	item->buf_len = 0;	

	if (tiny_queue_push(pnode->rssi_file_t->queue, item) != 0) {
		zlog_error(g_broker->log_handler,"Cannot push an 0 length element in the queue\n");
		free(item);
	}
	pnode->rssi_file_t->enable = 0;
}



/* -----------------------   test   --------------------------------------------- */
void test_process_exception(int state, g_broker_para* g_broker){
	int is_exception = 1;
	if(state == 1){	
		if(g_broker->system_ready == 0){	
			g_broker->system_ready = 1;	
			system_state_t* state = get_system_state(g_broker,NULL,g_broker->system_ready);
			char* response_json = system_state_response(g_broker->system_ready,is_exception,state);
			struct user_session_node *pnode = NULL;	
			list_for_each_entry(pnode, &g_broker->g_server->user_session_node_head, list) {	
				if(pnode->g_receive != NULL){    	
					assemble_frame_and_send(pnode->g_receive,response_json,strlen(response_json),TYPE_SYSTEM_STATE_EXCEPTION);	
				}	
			}	

            free(response_json);
			clear_system_state(state);	
		}else{	
            ; // do not inform node.js	
        }				
	}else if(state == 0){	
		g_broker->system_ready = 0;	
		char tmp_str[256];	
		sprintf(tmp_str, "device is not ready !");	
		zlog_info(g_broker->log_handler,"exception other msg : %s ", tmp_str);
		system_state_t* state = get_system_state(g_broker,tmp_str,g_broker->system_ready);
		char* response_json = system_state_response(g_broker->system_ready,is_exception,state);	

		struct user_session_node *pnode = NULL;	
		list_for_each_entry(pnode, &g_broker->g_server->user_session_node_head, list) {	
			if(pnode->g_receive != NULL){    	
				assemble_frame_and_send(pnode->g_receive,response_json,strlen(response_json),TYPE_SYSTEM_STATE_EXCEPTION);	
			}	
		}	
		zlog_info(g_broker->log_handler,"response_json: %s \n length = %d ", response_json, strlen(response_json));	
		free(response_json);	
		/* clear and reset system state variable */	
		clear_system_state(state);
	}	
}


