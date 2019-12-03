#ifndef DMA_HANDLER_H
#define DMA_HANDLER_H

#include "server.h"
#include "msg_queue.h"
#include "list.h"
#include "small_utility.h"

#include "zlog.h"

#define IQ_PAIR_NUM 256

typedef struct csi_spectrum_t{
    fftwf_complex *out_fft;
    fftwf_complex *in_IQ;
    fftwf_plan p;
    float spectrum[IQ_PAIR_NUM];
    float db_array[IQ_PAIR_NUM];
    float time_IQ[IQ_PAIR_NUM];
}csi_spectrum_t;

typedef struct g_dma_para{
	g_msg_queue_para*  g_msg_queue;
	g_server_para*     g_server;
	uint32_t           slow_cnt;            		   // for send cnt to slow datas
	int                enableCallback;
	int                csi_state;
    csi_spectrum_t*    csi_spectrum;

	int                constellation_state;
	void *             p_axidma;
	zlog_category_t*   log_handler;
}g_dma_para;

int create_dma_handler(g_dma_para** g_dma, g_server_para* g_server, zlog_category_t* handler);
int dma_register_callback(g_dma_para* g_dma);
void close_dma(g_dma_para* g_dma);


void start_csi(g_dma_para* g_dma);
void stop_csi(g_dma_para* g_dma);

void start_constellation(g_dma_para* g_dma);
void stop_constellation(g_dma_para* g_dma);

/* csi spectrum and time domain */
void processCSI(char* buf, int buf_len, g_dma_para* g_dma);



#endif//DMA_HANDLER_H



// void send_rssi_in_event_loop(char* buf, int buf_len, g_broker_para* g_broker);
// int open_rssi_state_external(int connfd, g_broker_para* g_broker); // control by external
// int close_rssi_state_external(int connfd, g_broker_para* g_broker); // control by external 
// int control_rssi_state(char *buf, int buf_len, g_broker_para* g_broker); // control by internal