#include "auto_log.h"
#include <string.h>


LogCollector logc;

/* --------- auto log save ----------------- */
static int autolog_file_num()
{
    FILE* fp = NULL; 
    int count = -1; 
    int BUFSZ = 100; 
    char buf[BUFSZ]; 
    char command[150]; 
 
    sprintf(command, "ls -l /run/media/mmcblk1p1/gw_web/web/auto_log | grep \"^-\" | wc -l"); 

    if((fp = popen(command,"r")) == NULL) 
    { 
        return -1;
    } 
    if((fgets(buf,BUFSZ,fp))!= NULL) 
    { 
        count = atoi(buf);
    }
	//printf("count = %d \n", count); 
    
    pclose(fp); 
    
    fp=NULL; 
    
	return count;
}

static int write_log (FILE* pFile, const char *format, ...) {
	va_list arg;
	int done;

	va_start (arg, format);
	//done = vfprintf (stdout, format, arg);

	time_t time_log = time(NULL);
	struct tm* tm_log = localtime(&time_log);
	fprintf(pFile, "%04d-%02d-%02d %02d:%02d:%02d ", tm_log->tm_year + 1900, tm_log->tm_mon + 1, tm_log->tm_mday, tm_log->tm_hour, tm_log->tm_min, tm_log->tm_sec);
	//2018-04-03 09:59:36 is running 10 55.550000
	done = vfprintf (pFile, format, arg);
	va_end (arg);

	fflush(pFile);
	return done;
}


void print_log(auto_log_item *item, LogCollector* logc){
    switch(item->type){
        case LOG_DATA:
        {
            log_data_t *data = (log_data_t*)(item->buf);
            write_log(logc->pfile_data, "%s %f,%f,%f\n", "rssi-snr-distance : ", data->rssi, data->snr, data->distance);
            break;
        }
        case LOG_HIGH_TEMP:
        {
            write_log(logc->pAlarmfile, "%s %s\n", "Alarm Event : high temperature --- ", item->buf);
            break;
        }
        case LOG_RADIO_EXCEPTION:
        {
            write_log(logc->pAlarmfile, "%s %s\n", "Alarm Event : radio system exception --- ", item->buf);
            break;
        }
    }
    if(item->buf!=NULL){
        free(item->buf);
    }
    free(item);
}

void* auto_save_thread(void* args){

	pthread_detach(pthread_self());

	LogCollector* tmp_logc = (LogCollector*)args;

	while(1){
		auto_log_item *item = tiny_queue_pop(tmp_logc->queue);
        print_log(item, tmp_logc);
	}
}

// external interface ----

int auto_log_start(LogCollector* logc,zlog_category_t* handler){
    logc->handler = handler;

    logc->pfile_data = NULL;
    logc->pAlarmfile = NULL;
    logc->queue = NULL;

    int id_count = autolog_file_num();
    id_count = id_count / 2;

	sprintf(logc->file_name, "/run/media/mmcblk1p1/gw_web/web/auto_log/data_state_%d.log", id_count);
	logc->pfile_data = fopen(logc->file_name, "a");
    if(logc->pfile_data == NULL){
        return -1;
    }

    sprintf(logc->file_name, "/run/media/mmcblk1p1/gw_web/web/auto_log/alarm_%d.log", id_count);
	logc->pAlarmfile = fopen(logc->file_name, "a");
    if(logc->pAlarmfile == NULL){
        return -2;
    }

    logc->queue  = tiny_queue_create();
    if (logc->queue == NULL) {
        return -3;
    }

	pthread_create(&(logc->thread_pid), NULL, auto_save_thread, (void*)(logc));
    return 0;
}

void auto_log_stop(LogCollector* logc){
	fclose(logc->pfile_data);
    logc->pfile_data = NULL;
    fclose(logc->pAlarmfile);
    logc->pAlarmfile = NULL;
}

int auto_save_data_log(LogCollector* logc, void* data, queue_item_type type){
    auto_log_item* item = (auto_log_item*)malloc(sizeof(auto_log_item));
    item->type = type;
    item->buf = data;

    if (tiny_queue_push(logc->queue, item) != 0){
		free(item);
        return -1; 
    }

    return 0;
}

int auto_save_alarm_log(LogCollector* logc, char* data, int data_len, queue_item_type type){
    auto_log_item* item = (auto_log_item*)malloc(sizeof(auto_log_item));
    item->type = type;
    item->buf = NULL;
    if(data_len!=0){
        item->buf = malloc(data_len);
        memcpy(item->buf,data,data_len);
    }

    if (tiny_queue_push(logc->queue, item) != 0){
		free(item);
        return -1; 
    }

    return 0;
}





