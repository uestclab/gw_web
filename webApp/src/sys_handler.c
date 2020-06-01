/* 
    1. process ip setting ..... 
    2. statistics eth and link .... 
*/

#include "sys_handler.h"
#include "response_json.h"

/**@defgroup System system_process_module.
* @{
* @ingroup system module
* @brief 提供网络统计信息数据. \n
* 响应网络设置操作
*/
/* ---------------------------------- statistics eth and link --------------------------------*/
int inquiry_statistics(g_receive_para* tmp_receive, g_broker_para* g_broker){
	char* response_json = statistics_response(g_broker->g_RegDev,g_broker->start_time,g_broker->update_acc_time,g_broker->log_handler);

	// zlog_info(g_broker->log_handler,"statistics_response : %s \n", response_json);
	assemble_frame_and_send(tmp_receive,response_json,strlen(response_json),TYPE_STATISTICS_RESPONSE);
	free(response_json);
}


int process_ip_setting(char* stat_buf, int stat_buf_len,zlog_category_t* handler){
	cJSON * root = NULL;
    cJSON * item = NULL;
    root = cJSON_Parse(stat_buf);
    item = cJSON_GetObjectItem(root,"ip");
	zlog_info(handler, "IP setting :%s \n", stat_buf);

    cJSON_Delete(root);
    return 0;
}

/** @} System*/