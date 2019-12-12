#ifndef DMA_HANDLER_H
#define DMA_HANDLER_H

#include "server.h"
#include "msg_queue.h"
#include "list.h"
#include "small_utility.h"
#include "web_common.h"

#include "zlog.h"

#define IQ_PAIR_NUM 256
#define CONSTELLATION_IQ_PAIR 10000

typedef struct constellation_module_t{
	int       constellation_state;
	int 	  user_cnt;
}constellation_module_t;

typedef struct constellation_iq_pair{
	int vectReal[CONSTELLATION_IQ_PAIR];
	int vectImag[CONSTELLATION_IQ_PAIR];
	int iq_cnt;
}constellation_iq_pair;

typedef struct csi_module_t{
	uint32_t           slow_cnt;            		   // for send cnt to slow datas
	int 			   csi_state;
	int 			   user_cnt;
	int                save_user_cnt;
}csi_module_t;

typedef struct csi_spectrum_t{
    fftwf_complex *out_fft;
    fftwf_complex *in_IQ;
    fftwf_plan p;
    float spectrum[IQ_PAIR_NUM];
    float db_array[IQ_PAIR_NUM];
    float time_IQ[IQ_PAIR_NUM];
	char           buf[1024];
	int            buf_len;
}csi_spectrum_t;

typedef struct g_dma_para{
	g_msg_queue_para*  g_msg_queue;
	g_server_para*     g_server;
	int                enableCallback;
    csi_spectrum_t*    csi_spectrum;
	csi_module_t       csi_module;
	struct list_head   csi_user_node_head;
	struct list_head   csi_save_user_node_head;

	constellation_iq_pair* cons_iq_pair;
	constellation_module_t constellation_module;
	struct list_head   constell_user_node_head;

	void *             p_axidma;
	zlog_category_t*   log_handler;
}g_dma_para;

typedef struct constell_user_node{
	g_dma_para*            g_dma;
	int                    connfd;
	struct list_head       list;
}constell_user_node;

typedef struct csi_user_node{
	g_dma_para*            g_dma;
	int                    connfd;
	int                    user_state;
	struct list_head       list;
}csi_user_node;

typedef struct csi_save_user_node{
	g_dma_para*            g_dma;
	int                    connfd;
	write_file_t*          csi_file_t;
	struct list_head       list;
}csi_save_user_node;

int create_dma_handler(g_dma_para** g_dma, g_server_para* g_server, zlog_category_t* handler);
int dma_register_callback(g_dma_para* g_dma);
void close_dma(g_dma_para* g_dma);


int start_csi(g_dma_para* g_dma); //control by internal
int stop_csi(g_dma_para* g_dma); //control by internal
int start_csi_state_external(int connfd, g_dma_para* g_dma); // control by external
int stop_csi_state_external(int connfd, g_dma_para* g_dma); // control by external

void send_csi_display_in_event_loop(g_dma_para* g_dma);
/* csi spectrum and time domain */
void processCSI(char* buf, int buf_len, g_dma_para* g_dma);
int process_csi_save_file(int connfd, char* stat_buf, int stat_buf_len, g_dma_para* g_dma);
void send_csi_to_save(g_dma_para* g_dma);
void clear_csi_write_status(csi_save_user_node* user_node, g_dma_para* g_dma);
void inform_stop_csi_write_thread(int connfd, g_dma_para* g_dma);

/* constellation function */
int start_constellation(g_dma_para* g_dma);
int stop_constellation(g_dma_para* g_dma);
int start_constellation_external(int connfd, g_dma_para* g_dma);
int stop_constellation_external(int connfd, g_dma_para* g_dma);
void send_constell_display_in_event_loop(g_dma_para* g_dma);

void processConstellation(char* buf, int buf_len, g_dma_para* g_dma);

#endif//DMA_HANDLER_H


