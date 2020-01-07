#ifndef __AUTO_LOG_H__
#define __AUTO_LOG_H__

#ifdef __cplusplus
    extern "C" {
#endif

#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <dirent.h>
#include <pthread.h> 

#include "tiny_queue.h"
#include "zlog.h"

typedef enum queue_item_type{
    LOG_DATA = 1,
    LOG_RSSI,
    LOG_SNR,
    LOG_DISTANCE,
    LOG_ALARM_EVENT, // boundary
    LOG_HIGH_TEMP,
    LOG_RADIO_EXCEPTION,
}queue_item_type;

typedef struct log_data_t{
    double rssi;
    double snr;
    double distance;
}log_data_t;

typedef struct auto_log_item{
    queue_item_type type;
    char*           buf;
}auto_log_item;

typedef struct LogCollector{
    zlog_category_t*    handler;
    pthread_t           thread_pid;
    char                file_name[256];
    FILE*               pfile_data;
    FILE*               pAlarmfile;
    tiny_queue_t*  		queue;
}LogCollector;

extern LogCollector logc;  // Global log collector for all
                             // single-threaded applications

int auto_log_start(LogCollector* logc, zlog_category_t* handler);

void auto_log_stop(LogCollector* logc);

int auto_save_data_log(LogCollector* logc, void* data, queue_item_type type);

int auto_save_alarm_log(LogCollector* logc, char* data, int data_len, queue_item_type type);

#ifdef __cplusplus
    }
#endif

#endif  /* __AUTO_LOG_H__ */


