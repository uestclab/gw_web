#include "threadFuncWrapper.h"
#include "small_utility.h"

void* inquiry_RF_state_wrapper(void* args){
    input_args_t* tmp_t = (input_args_t*)args;
    g_broker_para* g_broker = (g_broker_para*)tmp_t->arg_2;
	char* response_json = inquiry_rf_info(g_broker);
    postMsg(MST_RF_INFO_READY, response_json, strlen(response_json)+1, tmp_t->arg_1, 0, g_broker->g_msg_queue);
    free(response_json);
    free(tmp_t);
}

void postRfWorkToThreadPool(int user_node_id, g_broker_para* g_broker, ThreadPool* g_threadpool){

    input_args_t* tmp_t = (input_args_t*)malloc(sizeof(input_args_t));
    int* ptmp_id = (int*)malloc(sizeof(int));
    *ptmp_id = user_node_id;
    tmp_t->arg_1 = (void*)ptmp_id;
    tmp_t->arg_2 = (void*)g_broker;

	AddWorker(inquiry_RF_state_wrapper,(void*)tmp_t,g_threadpool);
}