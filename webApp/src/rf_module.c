/* 
    1. process RF info and setting .... dependence mosquitto_broker
*/
#include "rf_module.h"
#include "response_json.h"


int inquiry_rf_info(g_receive_para* tmp_receive, g_broker_para* g_broker){
	char* response_json = rf_info_response(g_broker->g_RegDev,g_broker->log_handler);
	zlog_info(g_broker->log_handler,"rf_info_response : %s \n", response_json);
	//assemble_frame_and_send(tmp_receive,response_json,strlen(response_json),TYPE_RF_INFO_RESPONSE);
	free(response_json);
}

int process_rf_freq_setting(char* stat_buf, int stat_buf_len, g_broker_para* g_broker){
	cJSON * root = NULL;
    cJSON * item = NULL;
    root = cJSON_Parse(stat_buf);
    item = cJSON_GetObjectItem(root,"frequency");
	int freq = item->valueint;
	cJSON_Delete(root);
	return 0;
}

int open_tx_power(){
    return 0;
}

int close_tx_power(){
    return 0;
}

int rx_gain_normal(){
    return 0;
}

int rx_gain_high(){
    return 0;
}