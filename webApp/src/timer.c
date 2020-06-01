#include "timer.h"
#include "small_utility.h"
#include "web_common.h"

/* */
void checkTaskToTimer(ngx_event_t *ev){
    g_msg_queue_para* g_msg_queue = (g_msg_queue_para*)(ev->data);
    postMsg(MSG_TIMEOUT, NULL, 0, NULL, 0, g_msg_queue);
    free(ev);
}

void addTimeOutWorkToTimer(void* data, event_timer_t* g_timer){
    ngx_event_t *ev = (ngx_event_t*)malloc(sizeof(ngx_event_t));
    ev->handler = checkTaskToTimer;
    ev->data = data;
    add_event_timer(ev, 120000, g_timer); // 60s
}


void logTask(ngx_event_t *ev){
    struct LogTaskTimer_t* tmp = (struct LogTaskTimer_t*)(ev->data);
    g_msg_queue_para* g_msg_queue = (g_msg_queue_para*)(tmp->data);
    *(tmp->cnt_num) = *(tmp->cnt_num) + 1;
    postMsg(MSG_TIMEOUT_TEST, NULL, 0, tmp, 0, g_msg_queue);
    free(ev);
}

void addLogTaskToTimer(void* data, int* num, event_timer_t* g_timer){
    ngx_event_t *ev = (ngx_event_t*)malloc(sizeof(ngx_event_t));
    ev->handler = logTask;

    struct LogTaskTimer_t* tmp = (struct LogTaskTimer_t*)malloc(sizeof(struct LogTaskTimer_t));
    tmp->data = data;
    tmp->cnt_num = num;

    ev->data = tmp;
    add_event_timer(ev, 30, g_timer); // 30ms
}