/* 
    1. process ip setting ..... 
    2. statistics eth and link .... 
*/

#include "sys_handler.h"
#include "response_json.h"

/* ---------------------------------- statistics eth and link --------------------------------*/
int inquiry_statistics(g_receive_para* tmp_receive, g_broker_para* g_broker){
	char* response_json = statistics_response(g_broker->g_RegDev,g_broker->start_time,g_broker->log_handler);

	zlog_info(g_broker->log_handler,"statistics_response : %s \n", response_json);
	//assemble_frame_and_send(tmp_receive,response_json,strlen(response_json),TYPE_STATISTICS_RESPONSE);
	free(response_json);
}


int process_ip_setting(char* stat_buf, int stat_buf_len,zlog_category_t* handler){
	cJSON * root = NULL;
    cJSON * item = NULL;
    root = cJSON_Parse(stat_buf);
    item = cJSON_GetObjectItem(root,"ip");
	//item->valuestring

    cJSON_Delete(root);
    return 0;
}

