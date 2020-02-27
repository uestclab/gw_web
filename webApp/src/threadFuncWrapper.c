#include "threadFuncWrapper.h"
#include "small_utility.h"

void* inquiry_RF_state_wrapper(void* args){
    input_args_t* tmp_t = (input_args_t*)args;
    g_receive_para* tmp_receive = (g_receive_para*)tmp_t->arg_1;
    g_broker_para* g_broker = (g_broker_para*)tmp_t->arg_2;
	char* response_json = inquiry_rf_info(tmp_receive, g_broker);
    postMsg(MST_RF_INFO_READY, response_json, strlen(response_json)+1, tmp_receive, 0, g_broker->g_msg_queue);
    free(response_json);
    free(tmp_t);
}

void postRfWorkToThreadPool(g_receive_para* tmp_receive, g_broker_para* g_broker, ThreadPool* g_threadpool){

    input_args_t* tmp_t = (input_args_t*)malloc(sizeof(input_args_t));
    tmp_t->arg_1 = (void*)tmp_receive;
    tmp_t->arg_2 = (void*)g_broker;

	AddWorker(inquiry_RF_state_wrapper,(void*)tmp_t,g_threadpool);
}