#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "mosquitto_broker.h"
#include "broker.h"
#include "cJSON.h"
#include "web_common.h"
#include "response_json.h"
#include "small_utility.h"

g_broker_para* g_broker_temp = NULL;

void assemble_frame_and_send(g_server_para* g_server, char* stat_buf, int stat_buf_len, int type){
	zlog_info(g_server->log_handler," stat_buf : %s",stat_buf);
	int length = stat_buf_len + FRAME_HEAD_ROOM;
	char* temp_buf = malloc(length);
	// htonl ?
	*((int32_t*)temp_buf) = (stat_buf_len + sizeof(int32_t));
	*((int32_t*)(temp_buf+ sizeof(int32_t))) = (type);
	memcpy(temp_buf + FRAME_HEAD_ROOM,stat_buf,stat_buf_len);
	//int ret = sendToPc(g_server, temp_buf, length, type);
	free(temp_buf);
}

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
	(*g_broker)->g_msg_queue       = NULL;
	(*g_broker)->log_handler       = handler;
    (*g_broker)->system_ready      = 0;
    (*g_broker)->g_RegDev          = g_RegDev;

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

int process_exception(char* buf, int buf_len, char *from, void* arg)
{ 
	int ret = 0;
	// if(g_broker_temp->g_server->waiting == STATE_DISCONNECTED)
	// 	return -1;
	if(strcmp(from,"mon/all/pub/system_stat") == 0){
		zlog_info(g_broker_temp->log_handler,"process_exception: mon/all/pub/system_stat , buf = %s \n", buf);
		//assemble_frame_and_send(g_broker_temp->g_server,buf,buf_len+1,41);
        // post msg 
	}else if(strcmp(from,"rf/all/pub/rssi") == 0){ 
		//print_rssi_struct(g_broker_temp,buf,buf_len); // send rssi struct stream to pc	
	}

	return ret;
}

int broker_register_callback_interface(g_broker_para* g_broker){
	int ret = register_callback("all", process_exception, "#");
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
            char* system_state_response_json = system_state_response(g_broker->system_ready, fpga_version, soft_version);
            /* ---------------------------------- */

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
		//sendLogToDisplay(tmp_str, strlen(tmp_str), panelHandle);
        // send msg to node js		
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
	return value;
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
}

void close_rssi(g_broker_para* g_broker){
	if(g_broker->rssi_state == 0)
		return;
	cJSON *root = cJSON_CreateObject();
	cJSON_AddStringToObject(root, "dev", "/dev/i2c-0");
	cJSON_AddStringToObject(root, "addr", "0x4a");
	cJSON_AddStringToObject(root, "force", "0x1");
	cJSON_AddStringToObject(root, "type", "rssi");
	cJSON_AddStringToObject(root, "timer", "0");
	cJSON_AddStringToObject(root, "pub_no", "1");
	cJSON_AddStringToObject(root, "dst", "rf");
	cJSON *array=cJSON_CreateArray();
	cJSON_AddItemToObject(root,"op_cmd",array);

	cJSON *arrayobj=cJSON_CreateObject();
	cJSON_AddItemToArray(array,arrayobj);
	cJSON_AddStringToObject(arrayobj, "_comment","rssi");
	cJSON_AddStringToObject(arrayobj, "cmd","0");
	cJSON_AddStringToObject(arrayobj, "reg","0x0a");
	cJSON_AddStringToObject(arrayobj, "size","1");

	char* close_rssi_jsonfile = cJSON_Print(root);

	control_rssi_state(close_rssi_jsonfile,strlen(close_rssi_jsonfile),g_broker);

	cJSON_Delete(root);
	free(close_rssi_jsonfile);
}


