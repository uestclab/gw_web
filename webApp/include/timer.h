#ifndef TIMER_H
#define TIMER_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include "zlog.h"
#include "event_timer.h"

typedef void (*TIMEOUT_FUN)(ngx_event_t *ev);

void addTimerTaskInterface(void* priv, TIMEOUT_FUN cb_ptr, int ms_cnt, event_timer_t* g_timer);

void addTimeOutWorkToTimer(void* data, event_timer_t* g_timer);


#endif//TIMER_H
