#include "threadFuncWrapper.h"
#include "small_utility.h"

/**@defgroup timer request.
* @{
* @ingroup timer module
* @brief timer basic process
*/
void* timeoutInquire(void* args){
    input_args_t* tmp_t = (input_args_t*)args;
    g_broker_para* g_broker = (g_broker_para*)tmp_t->arg_1;
    sleep(30);
    postMsg(MSG_TIMEOUT, NULL, 0, NULL, 0, g_broker->g_msg_queue);
    free(tmp_t);
}

void postTimeOutWorkToThreadPool(g_broker_para* g_broker, ThreadPool* g_threadpool){
    input_args_t* tmp_t = (input_args_t*)malloc(sizeof(input_args_t));
    tmp_t->arg_1 = (void*)g_broker;
	AddWorker(timeoutInquire,(void*)tmp_t,g_threadpool);
}




/**@defgroup process rf info request.
* @{
* @ingroup rf module
* @brief i2c操作较耗时，因此把request放入线程中操作，释放事件处理线程的时间
*/
void* inquiry_RF_state_wrapper(void* args){
    
    input_args_t* tmp_t = (input_args_t*)args;
    
    g_broker_para* g_broker = (g_broker_para*)tmp_t->arg_1;
	char* response_json = inquiry_rf_info(g_broker);

    web_msg_t* tmp_web = (web_msg_t*)malloc(sizeof(web_msg_t));
    tmp_web->arg_1 = tmp_t->number_1;
    tmp_web->buf_data = response_json;
    tmp_web->buf_data_len = strlen(response_json)+1;
    postMsg(MST_RF_INFO_READY, NULL, 0, tmp_web, 0, g_broker->g_msg_queue);
    free(tmp_t);
}

void postRfWorkToThreadPool(int user_node_id, g_broker_para* g_broker, ThreadPool* g_threadpool){

    input_args_t* tmp_t = (input_args_t*)malloc(sizeof(input_args_t));
    tmp_t->number_1 = user_node_id;
    tmp_t->arg_1 = (void*)g_broker;

	AddWorker(inquiry_RF_state_wrapper,(void*)tmp_t,g_threadpool);
}
