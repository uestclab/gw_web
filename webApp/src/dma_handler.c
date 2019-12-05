#include "cst_net.h"
#include "dma_handler.h"
#include "small_utility.h"
#include "response_json.h"


/* ----------------------- common use ------------------------- */

g_receive_para* findReceiveNode_dma(int connfd, g_dma_para* g_dma){
	struct user_session_node *pnode = NULL;
	g_receive_para* tmp_receive = NULL;
	list_for_each_entry(pnode, &g_dma->g_server->user_session_node_head, list) {
		if(pnode->g_receive->connfd == connfd){    
			break;
		}
	}
	tmp_receive = pnode->g_receive;
	return tmp_receive;
}

csi_user_node* findCsiUserNode(int connfd, g_dma_para* g_dma){
	struct csi_user_node *pnode = NULL;
	list_for_each_entry(pnode, &g_dma->csi_user_node_head, list) {
		if(pnode->connfd == connfd){    
			break;
		}
	}
	return pnode;
}

/* ----------------------- common use ------------------------- */


/* ========================================================================================== */
int IQ_register_callback(char* buf, int buf_len, void* arg)
{
	g_dma_para* g_dma = (g_dma_para*)arg;
    //zlog_info(g_dma->log_handler, " test point 1 : start IQ call_back , buf_len = %d \n", buf_len);
	if(g_dma->csi_module.slow_cnt < 50){
		g_dma->csi_module.slow_cnt = g_dma->csi_module.slow_cnt + 1;
		return 0;
	}
	g_dma->csi_module.slow_cnt = 0;

    //zlog_info(g_dma->log_handler, " test point 2 : start IQ call_back , buf_len = %d \n", buf_len);

	if(g_dma->csi_module.csi_state == 1 && g_dma->constellation_state == 0){
        //postMsg(MSG_CSI_READY, buf, buf_len, NULL, g_dma->g_msg_queue);
        if(buf_len >= 1024){
		    postMsg(MSG_CSI_READY, buf, 1024, NULL, g_dma->g_msg_queue);
            //zlog_info(g_dma->log_handler, " test point 3 : start IQ call_back  , time stop \n");
        }
	}else if(g_dma->constellation_state == 1 && g_dma->csi_module.csi_state == 0){
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
	//(*g_dma)->csi_state            = 0;                                           // csi_state
	(*g_dma)->constellation_state  = 0;                                           // constellation_state
	(*g_dma)->g_msg_queue          = g_server->g_msg_queue;
	(*g_dma)->p_axidma             = NULL;
	(*g_dma)->log_handler          = handler;
	//(*g_dma)->slow_cnt             = 0;

	(*g_dma)->p_axidma = axidma_open();
	if((*g_dma)->p_axidma == NULL){
		zlog_error(handler," In create_dma_handler -------- axidma_open failed !! \n");
		return -1;
	}

    g_dma_para* g_dma_tmp = (*g_dma);
    g_dma_tmp->csi_spectrum = (csi_spectrum_t*)malloc(sizeof(csi_spectrum_t));
    g_dma_tmp->csi_spectrum->in_IQ   = (fftwf_complex *)fftwf_malloc(sizeof(fftwf_complex) * IQ_PAIR_NUM);
    g_dma_tmp->csi_spectrum->out_fft = (fftwf_complex *)fftwf_malloc(sizeof(fftwf_complex) * IQ_PAIR_NUM);


	INIT_LIST_HEAD(&((*g_dma)->csi_user_node_head));
	INIT_LIST_HEAD(&((*g_dma)->csi_save_user_node_head));
	(*g_dma)->csi_module.csi_state = 0;
	(*g_dma)->csi_module.user_cnt  = 0;
	(*g_dma)->csi_module.slow_cnt  = 0;
	(*g_dma)->csi_module.save_user_cnt = 0;

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

// internel
int start_csi(g_dma_para* g_dma){
	int rc;
	if(g_dma->p_axidma == NULL){
		zlog_info(g_dma->log_handler,"g_dma->p_axidma == NULL start_csi() \n");
		g_dma->csi_module.csi_state = 0;
		return -1;
	}else{
		rc = axidma_chan(g_dma->p_axidma, 0x40);
		rc = axidma_start(g_dma->p_axidma);
		g_dma->csi_module.csi_state = 1;
		zlog_info(g_dma->log_handler,"rc = %d , start_csi() \n" , rc);
		return rc;
	}
}

int stop_csi(g_dma_para* g_dma){
	int rc;
	if(g_dma->p_axidma == NULL){
		zlog_info(g_dma->log_handler,"g_dma->p_axidma == NULL stop_csi\n");
		g_dma->csi_module.csi_state = 0;
		return -1;
	}else{
		rc = axidma_stop(g_dma->p_axidma);
		g_dma->csi_module.csi_state = 0;
		g_dma->constellation_state = 0;
		zlog_info(g_dma->log_handler,"rc = %d , stop_csi() \n" , rc);
		return rc;
	}
}

int start_csi_state_external(int connfd, g_dma_para* g_dma){
	int state;
	if(g_dma->csi_module.user_cnt == 0 && g_dma->csi_module.csi_state == 0){
		zlog_info(g_dma->log_handler,"start csi in start_csi() \n");
		state = start_csi(g_dma);
	}

    csi_user_node* new_node = (csi_user_node*)malloc(sizeof(csi_user_node));
	new_node->g_dma = g_dma;
    new_node->connfd = connfd;
	new_node->user_state = 0;

    list_add_tail(&new_node->list, &g_dma->csi_user_node_head);
	g_dma->csi_module.user_cnt++;
	zlog_info(g_dma->log_handler,"start csi user_cnt = %d \n",g_dma->csi_module.user_cnt);
	return 0;
}

/* call by close webpage or manually*/
int stop_csi_state_external(int connfd, g_dma_para* g_dma){
    struct list_head *pos, *n;
    struct csi_user_node *pnode = NULL;
    list_for_each_safe(pos, n, &g_dma->csi_user_node_head){
        pnode = list_entry(pos, struct csi_user_node, list);
        if(pnode->connfd == connfd){
            list_del(pos);
            g_dma->csi_module.user_cnt--;
			zlog_info(g_dma->log_handler,"find node in stop_csi_state_external() ! connfd = %d, user_cnt = %d ", connfd,g_dma->csi_module.user_cnt);
            break;
        }
    }

	if(pnode != NULL){
		// free pnode{
		zlog_info(g_dma->log_handler," stop_csi_state_external : delete from list , free this node 0x%x \n", pnode);
		free(pnode);
	}

	if(g_dma->csi_module.user_cnt == 0 && g_dma->csi_module.csi_state == 1){
		zlog_info(g_dma->log_handler,"stop csi in stop_csi() \n");
		stop_csi(g_dma);
	}

	return 0;
}

void send_csi_display_in_event_loop(g_dma_para* g_dma){

	char* csi_data_response_json = csi_data_response(g_dma->csi_spectrum->db_array, g_dma->csi_spectrum->time_IQ,256);
	//zlog_info(g_dma->log_handler, "csi res data : %s", csi_data_response_json);

	struct csi_user_node *pnode = NULL;
	list_for_each_entry(pnode, &g_dma->csi_user_node_head, list) {
		g_receive_para* tmp_receive = findReceiveNode_dma(pnode->connfd,g_dma);
		if(tmp_receive != NULL){
			zlog_info(g_dma->log_handler, "send csi to node js , json len : %d \n", strlen(csi_data_response_json));
			//assemble_frame_and_send(tmp_receive,csi_data_response_json,strlen(csi_data_response_json),TYPE_CSI_DATA_RESPONSE);
		}
	}

	free(csi_data_response_json);	

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
