#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "mosquitto_broker.h"
#include "cJSON.h"
#include "web_common.h"
#include "response_json.h"
#include "small_utility.h"

g_broker_para* g_broker_temp = NULL;

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
	(*g_broker)->rssi_state        = 0;                                           // rssi_stat
	(*g_broker)->g_msg_queue       = g_server->g_msg_queue;
	(*g_broker)->log_handler       = handler;
    (*g_broker)->system_ready      = 0;
    (*g_broker)->g_RegDev          = g_RegDev;
	(*g_broker)->enableCallback    = 0;

	g_broker_para* tmp_broker = *g_broker;
	tmp_broker->rssi_file_t         = (write_file_t*)malloc(sizeof(write_file_t));
	pthread_mutex_init(&(tmp_broker->rssi_file_t->mutex),NULL);

	(*g_broker)->rssi_file_t->enable = 0;
	(*g_broker)->rssi_file_t->file   = NULL;
	(*g_broker)->rssi_file_t->queue  = NULL;

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

int inform_exception(char* buf, int buf_len, char *from, void* arg)
{ 
	int ret = 0;
	if(strcmp(from,"mon/all/pub/system_stat") == 0){
		zlog_info(g_broker_temp->log_handler,"inform_exception: mon/all/pub/system_stat , buf = %s \n", buf);
        postMsg(MSG_SYSTEM_STATE_EXCEPTION, buf, buf_len, g_broker_temp->g_msg_queue);	
	}else if(strcmp(from,"rf/all/pub/rssi") == 0){ 
		print_rssi_struct(g_broker_temp,buf,buf_len); // send rssi struct stream to pc	
	}
	return ret;
}

void process_exception(char* stat_buf, int stat_buf_len, g_broker_para* g_broker){
	cJSON * root = NULL;
    cJSON * item = NULL;
    root = cJSON_Parse(stat_buf);
    item = cJSON_GetObjectItem(root,"stat");
	if(strcmp(item->valuestring,"0x20") == 0 || strcmp(item->valuestring,"0x80000020") == 0){
		if(g_broker->system_ready == 0){
			g_broker->system_ready = 1;
            char* soft_version = c_compiler_builtin_macro();
            uint32_t value = get_fpga_version(g_broker->g_RegDev);
            char* fpga_version = parse_fpga_version(value);
			char* system_state_response_json = system_state_response(g_broker->system_ready, fpga_version, soft_version,1);
			assemble_frame_and_send(g_broker->g_server,system_state_response_json,strlen(system_state_response_json)+1,MSG_SYSTEM_STATE_EXCEPTION);
            free(system_state_response_json);
		}else{
            ;
        }			
	}else{
		g_broker->system_ready = 0;
		char tmp_str[256];
		sprintf(tmp_str, "device is not ready , SystemState not 0x20 !!! %s \n", item->valuestring);
		zlog_info(g_broker->log_handler,"exception other msg : %s ", tmp_str);
		char* response_json = system_state_response(g_broker->system_ready, tmp_str, NULL, 1);
		assemble_frame_and_send(g_broker->g_server,response_json,strlen(response_json)+1,MSG_SYSTEM_STATE_EXCEPTION);

		/* clear and reset system state variable */

		free(response_json);	
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

void parse_system_state(char* stat_buf, int stat_buf_len, g_broker_para* g_broker){
	cJSON * root = NULL;
    cJSON * item = NULL;
    root = cJSON_Parse(stat_buf);
    item = cJSON_GetObjectItem(root,"stat");
	if(strcmp(item->valuestring,"0x20") == 0 || strcmp(item->valuestring,"0x80000020") == 0){
		if(g_broker->system_ready == 0){
			g_broker->system_ready = 1;
            // send frame to node js
            char* soft_version = c_compiler_builtin_macro();
            uint32_t value = get_fpga_version(g_broker->g_RegDev);
            char* fpga_version = parse_fpga_version(value);
			char* system_state_response_json = system_state_response(g_broker->system_ready, fpga_version, soft_version,0);
			assemble_frame_and_send(g_broker->g_server,system_state_response_json,strlen(system_state_response_json)+1,TYPE_SYSTEM_STATE_RESPONSE);
            free(soft_version);
            free(fpga_version);
            free(system_state_response_json);
		}else{
            ;
        }			
	}else{
		g_broker->system_ready = 0;
		char tmp_str[256];
		sprintf(tmp_str, "device is not ready , SystemState not 0x20 !!! %s \n", item->valuestring);
		zlog_info(g_broker->log_handler,"other msg : %s ", tmp_str);
		char* response_json = system_state_response(g_broker->system_ready, tmp_str, NULL, 0);
		assemble_frame_and_send(g_broker->g_server,response_json,strlen(response_json)+1,TYPE_SYSTEM_STATE_RESPONSE);
		free(response_json);	
	}
	cJSON_Delete(root);
    free(stat_buf);
}

int inquiry_system_state(g_broker_para* g_broker){
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
        parse_system_state(stat_buf,stat_buf_len,g_broker);
	}
	return ret;
}

int inquiry_reg_state(g_broker_para* g_broker){
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

	assemble_frame_and_send(g_broker->g_server,reg_state_response_json,strlen(reg_state_response_json)+1,TYPE_REG_STATE_RESPONSE);

	free(reg_state);
	free(reg_state_response_json);
}

int inquiry_dac_state(){
	int value = gpio_read(973);
	printf("dac state : %d \n",value);
	return value;
}

// ---------- rssi ----------------
/* value as input , cmd control rw*/
int threadsafe_rw_enable_rssi(g_broker_para* g_broker, int value, int cmd){
	pthread_mutex_lock(&g_broker->rssi_file_t->mutex);
	if(cmd == WRITE){
		g_broker->rssi_file_t->enable = value;
		pthread_mutex_unlock(&g_broker->rssi_file_t->mutex); // note that: pthread_mutex_unlock before return !!!!! 
		return 0;
	}
	int ret = g_broker->rssi_file_t->enable;
	pthread_mutex_unlock(&g_broker->rssi_file_t->mutex);
	return ret;
}


int control_rssi_state(char *buf, int buf_len, g_broker_para* g_broker){
	zlog_info(g_broker->log_handler,"rssi json = %s \n",buf);
	int ret = -1;
	char* stat_buf = NULL;
	int stat_buf_len = 0;

	cJSON * root = NULL;
    cJSON * item = NULL;
    root = cJSON_Parse(buf);
    item = cJSON_GetObjectItem(root,"dst"); // different device is a dst

	ret = dev_transfer(buf, buf_len, &stat_buf, &stat_buf_len, item->valuestring, -1);

	if(ret == 0 && stat_buf_len > 0){
		item = cJSON_GetObjectItem(root,"timer");
		if(strcmp(item->valuestring,"0") == 0)
			g_broker->rssi_state = 0;
		else
			g_broker->rssi_state = 1;
		zlog_info(g_broker->log_handler,"rssi_state = %d \n, rssi return json = %s \n", g_broker->rssi_state , stat_buf);
		free(stat_buf);
	}
	cJSON_Delete(root);
	return ret;
}

void print_rssi_struct(g_broker_para* g_broker, char* buf, int buf_len){
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
	assemble_frame_and_send(g_broker->g_server,rssi_data_response_json,strlen(rssi_data_response_json)+1,TYPE_RSSI_DATA_RESPONSE);
	free(rssi_data_response_json);

	int state = threadsafe_rw_enable_rssi(g_broker,0,READ);

	if(state == 0){
		return;
	}else if(state == 1){ /* for write file */
		int msg_len = 4 + 4 + 4 + tmp_buf->rssi_buf_len;
		char* rssi_buf = malloc(msg_len);
		*((int32_t*)rssi_buf) = tmp_buf->timestamp.tv_sec;
		*((int32_t*)(rssi_buf+ sizeof(int32_t) )) = tmp_buf->timestamp.tv_usec;
		*((int32_t*)(rssi_buf+ sizeof(int32_t) * 2)) = tmp_buf->rssi_buf_len;
		memcpy(rssi_buf + sizeof(int32_t) * 3, tmp_buf->rssi_buf, tmp_buf->rssi_buf_len);

		queue_item* item = (queue_item*)malloc(sizeof(queue_item));
		item->buf = rssi_buf;
		item->buf_len = msg_len;	

		if (tiny_queue_push(g_broker->rssi_file_t->queue, item) != 0) {
			zlog_error(g_broker->log_handler,"Cannot push an element in the queue\n");
			free(rssi_buf);
		}

	}else if(state == 2){
		queue_item* item = (queue_item*)malloc(sizeof(queue_item));
		item->buf = NULL;
		item->buf_len = 0;	

		if (tiny_queue_push(g_broker->rssi_file_t->queue, item) != 0) {
			zlog_error(g_broker->log_handler,"Cannot push an 0 length element in the queue\n");
		}
		threadsafe_rw_enable_rssi(g_broker,0,WRITE);
	}
}

void* rssi_write_thread(void* args){

	pthread_detach(pthread_self());

	g_broker_para* g_broker = (g_broker_para*)args;

	zlog_info(g_broker->log_handler,"start rssi_write_thread()\n");

	while(1){
		queue_item *work_item = tiny_queue_pop(g_broker->rssi_file_t->queue); // need work length

		if(work_item->buf_len == 0){
			free(work_item);
			break;
		}

		char* work = work_item->buf;
		int32_t timestamp_tv_sec = *((int32_t*)work);
		int32_t timestamp_tv_usec = *((int32_t*)(work+ sizeof(int32_t) ));
		int32_t rssi_buf_len = *((int32_t*)(work+ sizeof(int32_t) * 2));
		printf("timestamp : sec = %d , usec = %d , rssi_len = %d \n", timestamp_tv_sec, timestamp_tv_usec, rssi_buf_len);
		fwrite(work,sizeof(char), work_item->buf_len, g_broker->rssi_file_t->file);
		free(work);
		free(work_item);
	}

	postMsg(MSG_CLEAR_RSSI_WRITE_STATUS,NULL,0,g_broker->g_msg_queue);
	zlog_info(g_broker->log_handler,"end Exit rssi_write_thread()\n");
}

int process_rssi_save_file(char* stat_buf, int stat_buf_len, g_broker_para* g_broker){
	cJSON * root = NULL;
    cJSON * item = NULL;
    root = cJSON_Parse(stat_buf);
    item = cJSON_GetObjectItem(root,"type");
	if(item->valueint != TYPE_RSSI_CONTROL){
		cJSON_Delete(root);
		return -1;
	}

	item = cJSON_GetObjectItem(root,"op_cmd");
	if(item->valueint == 0){ /* stop save */
		threadsafe_rw_enable_rssi(g_broker,2,WRITE);
	}else if(item->valueint == 1){ /* start save */
		item = cJSON_GetObjectItem(root,"file_name");
		sprintf(g_broker->rssi_file_t->file_name, "/run/media/mmcblk1p1/handover_test/web/log/%s", item->valuestring);
		printf("file_name : %s\n",g_broker->rssi_file_t->file_name);
		g_broker->rssi_file_t->file = fopen(g_broker->rssi_file_t->file_name,"wb");
		if(g_broker->rssi_file_t->file == NULL){
			zlog_error(g_broker->log_handler,"Cannot creare the rssi queue\n");
			cJSON_Delete(root);
			return -1;
		}

		g_broker->rssi_file_t->queue  = tiny_queue_create();
		if (g_broker->rssi_file_t->queue == NULL) {
			zlog_error(g_broker->log_handler,"Cannot creare the rssi queue\n");
			cJSON_Delete(root);
			return -1;
		}

		pthread_t thread_pid;
		pthread_create(&thread_pid, NULL, rssi_write_thread, (void*)(g_broker));
		threadsafe_rw_enable_rssi(g_broker,1,WRITE);
	}
	return 0;
}

void clear_rssi_write_status(g_broker_para* g_broker){
	/* close file */
	fclose(g_broker->rssi_file_t->file);
	g_broker->rssi_file_t->file = NULL;

	/* destroy queue */
	tiny_queue_destory(g_broker->rssi_file_t->queue);
}