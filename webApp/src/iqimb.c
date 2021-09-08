
#include "iqimb.h"

void iqimb_check_task(ngx_event_t *ev){
    // iqimb_t* tmp_priv = (iqimb_t*)(ev->data);

    // postMsg(MSG_IQ_IMB_APP_TIMEOUT,NULL,0,NULL,0,tmp_priv->g_server->g_msg_queue);

    // free(tmp_priv);
    // free(ev);
}


void get_local_time(char *time_str, int len, struct timeval *tv)
{
    struct tm* ptm;    
    ptm = localtime (&(tv->tv_sec));
 
    /* 格式化日期和时间，精确到秒为单位。*/
    //strftime (time_string, sizeof(time_string), "%Y/%m/%d %H:%M:%S", ptm); //输出格式为: 2018/12/09 10:48:31.391
    //strftime (time_string, sizeof(time_string), "%Y|%m|%d %H-%M-%S", ptm); //输出格式为: 2018|12|09 10-52-28.302
    //strftime (time_string, sizeof(time_string), "%Y-%m-%d %H:%M:%S", ptm); //输出格式为: 2018-12-09 10:52:57.200
    strftime (time_str, len, "%Y_%m_%d_%H_%M_%S", ptm); //输出格式为: 2018\12\09 10-52-28.302
}

int iqimb_start_cycle(g_server_para* g_server, int cnt_s){
    iqimb_t* tmp_priv = (iqimb_t*)malloc(sizeof(iqimb_t));
    tmp_priv->g_server = g_server;
    tmp_priv->file = NULL;

    //get time 
    // char local_time_str[128];
    // struct timeval tv;
    // gettimeofday(&tv, NULL);

    // get_local_time(local_time_str,sizeof(local_time_str),&tv);

    addTimerTaskInterface((void*)tmp_priv, iqimb_check_task, cnt_s*1000, g_server->g_timer);

    return 0;
}

int transfer_data_to_iqimb(char* buf, int buf_len, g_dma_para* g_dma, g_server_para* g_server){
    // float out_512[512];
    // parse_IQ_to_iqimb(buf, buf_len, out_512);

    // //clear file
    // int fd = open(g_dma->iqimb_module.file_name,O_RDWR);
    // if(fd < 0){
    //     zlog_error(g_server->log_handler,"open file -- clear file failed\n");
    // }else{
    //     zlog_info(g_server->log_handler,"clear file successful\n");

    //     /* 清空文件 */
    //     ftruncate(fd,0);

    //     /* 重新设置文件偏移量 */
    //     lseek(fd,0,SEEK_SET);

    //     close(fd);
    // }

    // // write to file
    // g_dma->iqimb_module.file = fopen(g_dma->iqimb_module.file_name,"w");
    // if(g_dma->iqimb_module.file == NULL){
    //     zlog_error(g_server->log_handler,"Cannot create the /run/media/mmcblk1p2/iqswap/raw_data.txt\n");
    //     return -1;
    // }

    // for(int i=0 ;i<512;i++){
    //     // zlog_error(g_server->log_handler, " out_512[%d] = %f ", i, out_512[i]);
    //     fprintf(g_dma->iqimb_module.file, "%f \n",out_512[i]); 
    // }

    // fclose(g_dma->iqimb_module.file);
    // g_dma->iqimb_module.file = NULL;

    // system("iq_imbalance /run/media/mmcblk1p2/raw_data.txt /run/media/mmcblk1p1/etc/zlog_default.conf");
    // zlog_info(g_server->log_handler, "Remote Procedure Call iq_imbalance \n");
    
    return 0;
}
