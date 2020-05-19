#ifndef TIMER_H
#define TIMER_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include "zlog.h"
#include "event_timer.h"


void addTimeOutWorkToTimer(void* data, event_timer_t* g_timer);

/* test code */
struct LogTaskTimer_t{
    int* cnt_num;
    void* data;
};
void addLogTaskToTimer(void* data, int *num, event_timer_t* g_timer);

#endif//TIMER_H
