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

/* Timer Task Interface */
void addTimerTaskInterface(void* priv, TIMEOUT_FUN cb_ptr, int ms_cnt, event_timer_t* g_timer){
    ngx_event_t *ev = (ngx_event_t*)malloc(sizeof(ngx_event_t));
    ev->handler = cb_ptr;
    ev->data = priv;
    add_event_timer(ev, ms_cnt, g_timer); // 60s
}