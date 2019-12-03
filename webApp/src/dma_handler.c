#include "cst_net.h"
#include "dma_handler.h"
#include "web_common.h"
#include "small_utility.h"

/* ========================================================================================== */
int IQ_register_callback(char* buf, int buf_len, void* arg)
{
	g_dma_para* g_dma = (g_dma_para*)arg;
    //zlog_info(g_dma->log_handler, " test point 1 : start IQ call_back , buf_len = %d \n", buf_len);
	if(g_dma->slow_cnt < 50){
		g_dma->slow_cnt = g_dma->slow_cnt + 1;
		return 0;
	}
	g_dma->slow_cnt = 0;

    zlog_info(g_dma->log_handler, " test point 2 : start IQ call_back , buf_len = %d \n", buf_len);

	if(g_dma->csi_state == 1 && g_dma->constellation_state == 0){
        //postMsg(MSG_CSI_READY, buf, buf_len, NULL, g_dma->g_msg_queue);
        if(buf_len >= 1024){
		    postMsg(MSG_CSI_READY, buf, 1024, NULL, g_dma->g_msg_queue);
            zlog_info(g_dma->log_handler, " test point 3 : start IQ call_back  , time stop \n");
        }
	}else if(g_dma->constellation_state == 1 && g_dma->csi_state == 0){
        //postMsg(MSG_CONSTELLATION_READY, buf, buf_len, NULL, g_dma->g_msg_queue);
        postMsg(MSG_CONSTELLATION_READY, NULL, 0, NULL, g_dma->g_msg_queue);
	}

	return 0;
}
/* need release fft buffer? */
// fftwf_destroy_plan(p);
// fftwf_free(in);
// fftwf_free(out);

int create_dma_handler(g_dma_para** g_dma, g_server_para* g_server, zlog_category_t* handler){

	zlog_info(handler,"create create_dma_handler() \n");
	*g_dma = (g_dma_para*)malloc(sizeof(struct g_dma_para));
	(*g_dma)->g_server      	   = g_server;                                    //
	(*g_dma)->enableCallback       = 0;
	(*g_dma)->csi_state            = 0;                                           // csi_state
	(*g_dma)->constellation_state  = 0;                                           // constellation_state
	(*g_dma)->g_msg_queue          = g_server->g_msg_queue;
	(*g_dma)->p_axidma             = NULL;
	(*g_dma)->log_handler          = handler;
	(*g_dma)->slow_cnt             = 0;

	(*g_dma)->p_axidma = axidma_open();
	if((*g_dma)->p_axidma == NULL){
		zlog_error(handler," In create_dma_handler -------- axidma_open failed !! \n");
		return -1;
	}

    g_dma_para* g_dma_tmp = (*g_dma);
    g_dma_tmp->csi_spectrum = (csi_spectrum_t*)malloc(sizeof(csi_spectrum_t));
    g_dma_tmp->csi_spectrum->in_IQ   = (fftwf_complex *)fftwf_malloc(sizeof(fftwf_complex) * IQ_PAIR_NUM);
    g_dma_tmp->csi_spectrum->out_fft = (fftwf_complex *)fftwf_malloc(sizeof(fftwf_complex) * IQ_PAIR_NUM);

	zlog_info(handler,"End create_dma_handler open axidma() p_csi = %x, \n", (*g_dma)->p_axidma);
	return 0;

}

int dma_register_callback(g_dma_para* g_dma){
	int rc;
    if(g_dma->p_axidma == NULL){
        zlog_error(g_dma->log_handler,"g_dma->p_axidma == NULL , register failed !\n");
        return -2;
    }
	rc = axidma_register_callback(g_dma->p_axidma, IQ_register_callback, (void*)(g_dma));
	if(rc != 0){
		zlog_error(g_dma->log_handler," axidma_register_callback failed\n");
		return -1;
	}
	return 0;
}

void close_dma(g_dma_para* g_dma){
	if(g_dma->p_axidma == NULL){
		zlog_info(g_dma->log_handler,"g_dma->p_axidma == NULL in close_dma\n");
		return;
	}
	zlog_info(g_dma->log_handler,"before axidma_close : %x \n" , g_dma->p_axidma);
	axidma_close(g_dma->p_axidma);
	g_dma->p_axidma = NULL;
	zlog_info(g_dma->log_handler,"close_dma() \n");
}

void start_csi(g_dma_para* g_dma){
	int rc;
	if(g_dma->csi_state == 1){
		zlog_info(g_dma->log_handler,"axidma is already start\n");
		return;
	}
	if(g_dma->p_axidma == NULL){
		zlog_info(g_dma->log_handler,"g_dma->p_axidma == NULL start_csi() \n");
		g_dma->csi_state = 0;
		return;
	}else{
		rc = axidma_chan(g_dma->p_axidma, 0x40);
		rc = axidma_start(g_dma->p_axidma);
		g_dma->csi_state = 1;
		zlog_info(g_dma->log_handler,"rc = %d , start_csi() \n" , rc);
	}
}

void stop_csi(g_dma_para* g_dma){
	int rc;
	if(g_dma->csi_state == 0){
		zlog_info(g_dma->log_handler,"csi is already stop\n");
		return;
	}
	if(g_dma->p_axidma == NULL){
		zlog_info(g_dma->log_handler,"g_dma->p_axidma == NULL stop_csi\n");
		g_dma->csi_state = 0;
		return;
	}else{
		rc = axidma_stop(g_dma->p_axidma);
		g_dma->csi_state = 0;
		g_dma->constellation_state = 0;
		zlog_info(g_dma->log_handler,"rc = %d , stop_csi() \n" , rc);
	}
}

void start_constellation(g_dma_para* g_dma){
	int rc;
	if(g_dma->constellation_state == 1){
		zlog_info(g_dma->log_handler,"constellation is already start\n");
		return;
	}
	if(g_dma->p_axidma == NULL){
		zlog_info(g_dma->log_handler,"g_dma->p_axidma == NULL start_constellation\n");
		g_dma->constellation_state = 0;
		return;
	}else{
		rc = axidma_chan(g_dma->p_axidma, 0x80);
		rc = axidma_start(g_dma->p_axidma);
		g_dma->constellation_state = 1;
		zlog_info(g_dma->log_handler,"rc = %d , start_constellation() \n" , rc);
	}	
}

void stop_constellation(g_dma_para* g_dma){
	int rc;
	if(g_dma->constellation_state == 0){
		zlog_info(g_dma->log_handler,"constellation is already stop\n");
		return;
	}
	if(g_dma->p_axidma == NULL){
		zlog_info(g_dma->log_handler,"g_dma->p_axidma == NULL stop_constellation\n");
		g_dma->constellation_state = 0;
		return;
	}else{
		rc = axidma_stop(g_dma->p_axidma);
		g_dma->constellation_state = 0;
		g_dma->csi_state = 0;
		zlog_info(g_dma->log_handler,"rc = %d , stop_constellation() \n" , rc);
	}
}


/* ---------------------- CSI function ---------------------------- */

// buf_len == 1024 must /* all compare with before log data*/
void processCSI(char* buf, int buf_len, g_dma_para* g_dma){
    parse_IQ_from_net(buf, buf_len, g_dma->csi_spectrum->in_IQ);
    calculate_spectrum(g_dma->csi_spectrum->in_IQ, g_dma->csi_spectrum->out_fft, &(g_dma->csi_spectrum->p), g_dma->csi_spectrum->spectrum, 256); 
    myfftshift(g_dma->csi_spectrum->db_array, g_dma->csi_spectrum->spectrum, 256);
    timeDomainChange(g_dma->csi_spectrum->in_IQ, g_dma->csi_spectrum->time_IQ, 256);
}
