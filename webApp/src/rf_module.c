/* 
    1. process RF info and setting .... dependence mosquitto_broker
*/
#include "rf_module.h"
#include "response_json.h"
#include "small_utility.h"

int frequency_rf;
int tx_power_state;
int rx_gain_state;

/**@defgroup RF rf_process_module.
* @{
* @ingroup rf module
* @brief 提供射频信息数据. \n
* 响应设置射频等操作
*/
int inquiry_rf_info(g_receive_para* tmp_receive, g_broker_para* g_broker){
	char* response_json = rf_info_response(g_broker->g_RegDev,g_broker->log_handler);
	//zlog_info(g_broker->log_handler,"rf_info_response : %s \n", response_json);
	assemble_frame_and_send(tmp_receive,response_json,strlen(response_json),TYPE_RF_INFO_RESPONSE);
	free(response_json);
}

int process_rf_freq_setting(char* stat_buf, int stat_buf_len, g_broker_para* g_broker){
    zlog_info(g_broker->log_handler, "rf frequency setting :%s \n", stat_buf);
	cJSON * root = NULL;
    cJSON * item = NULL;
    root = cJSON_Parse(stat_buf);
    item = cJSON_GetObjectItem(root,"frequency");
    frequency_rf = stringToDecimalInt(item->valuestring);
    zlog_info(g_broker->log_handler,"item->valuestring = %s , %d", item->valuestring, frequency_rf);
	cJSON_Delete(root);
	return 0;
}

int open_tx_power(){
    tx_power_state = 1;
    return 0;
}

int close_tx_power(){
    tx_power_state = 0;
    return 0;
}

int rx_gain_normal(){
    rx_gain_state = 0;
    return 0;
}

int rx_gain_high(){
    rx_gain_state = 1;
    return 0;
}

/** @} RF*/
