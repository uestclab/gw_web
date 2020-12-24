#include "cst_net.h"
#include "dma_handler.h"
#include "small_utility.h"
#include "response_json.h"


/* ----------------------- common use ------------------------- */
csi_user_node* findCsiUserNode(int connfd, g_dma_para* g_dma){
	struct csi_user_node *pnode = NULL;
	struct csi_user_node *tmp = NULL;
	list_for_each_entry(tmp, &g_dma->csi_user_node_head, list) {
		if(tmp->connfd == connfd){ 
			pnode = tmp;   
			break;
		}
	}
	return pnode;
}

csi_save_user_node* findCsiSaveUserNode(int connfd, g_dma_para* g_dma){
	struct csi_save_user_node *pnode = NULL;
	struct csi_save_user_node *tmp = NULL;
	list_for_each_entry(tmp, &g_dma->csi_save_user_node_head, list) {
		if(tmp->connfd == connfd){
			pnode = tmp;    
			break;
		}
	}
	return pnode;
}

constell_user_node* findConstellUserNode(int connfd, g_dma_para* g_dma){
	struct constell_user_node *pnode = NULL;
	struct constell_user_node *tmp = NULL;
	list_for_each_entry(tmp, &g_dma->constell_user_node_head, list) {
		if(tmp->connfd == connfd){ 
			pnode = tmp;   
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

	usleep(10000);

	g_dma->csi_module.slow_cnt = 0;

	if(g_dma->csi_module.csi_state == 1 && g_dma->constellation_module.constellation_state == 0){
        if(buf_len >= 1024){
		    postMsg(MSG_CSI_READY, buf + 8, 1024, NULL, 0, g_dma->g_msg_queue); // empty 8 byte in front of buf , buf_len indicate IQ length --- note
        }
	}else if(g_dma->constellation_module.constellation_state == 1 && g_dma->csi_module.csi_state == 0){
		int tmp_data_len = buf_len;
		if(buf_len > 2000)
			tmp_data_len = 2000;
		char* tmp_data = malloc(tmp_data_len);
		memcpy(tmp_data, buf + 8, tmp_data_len);
        postMsg(MSG_CONSTELLATION_READY, NULL, 0, tmp_data, tmp_data_len, g_dma->g_msg_queue);
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
	(*g_dma)->g_msg_queue          = g_server->g_msg_queue;
	(*g_dma)->p_axidma             = NULL;
	(*g_dma)->log_handler          = handler;

	(*g_dma)->p_axidma = axidma_open();
	if((*g_dma)->p_axidma == NULL){
		zlog_error(handler," In create_dma_handler -------- axidma_open failed !! \n");
		return -1;
	}

    g_dma_para* g_dma_tmp = (*g_dma);
    g_dma_tmp->csi_spectrum = (csi_spectrum_t*)malloc(sizeof(csi_spectrum_t));
    g_dma_tmp->csi_spectrum->in_IQ   = (fftwf_complex *)fftwf_malloc(sizeof(fftwf_complex) * IQ_PAIR_NUM);
    g_dma_tmp->csi_spectrum->out_fft = (fftwf_complex *)fftwf_malloc(sizeof(fftwf_complex) * IQ_PAIR_NUM);
	//g_dma_tmp->csi_spectrum->buf = NULL;
	g_dma_tmp->csi_spectrum->buf_len = 0;


	INIT_LIST_HEAD(&((*g_dma)->csi_user_node_head));
	INIT_LIST_HEAD(&((*g_dma)->csi_save_user_node_head));
	(*g_dma)->csi_module.csi_state = 0;
	(*g_dma)->csi_module.user_cnt  = 0;
	(*g_dma)->csi_module.slow_cnt  = 0;
	(*g_dma)->csi_module.save_user_cnt = 0;


	g_dma_tmp->cons_iq_pair = (constellation_iq_pair*)malloc(sizeof(constellation_iq_pair));
	g_dma_tmp->cons_iq_pair->iq_cnt = 0;
	INIT_LIST_HEAD(&(g_dma_tmp->constell_user_node_head));
	g_dma_tmp->constellation_module.constellation_state = 0;
	g_dma_tmp->constellation_module.user_cnt = 0;


	zlog_info(handler,"End create_dma_handler open axidma() p_csi = 0x%x, \n", (*g_dma)->p_axidma);
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
	zlog_info(g_dma->log_handler,"before axidma_close : 0x%x \n" , g_dma->p_axidma);
	axidma_close(g_dma->p_axidma);
	g_dma->p_axidma = NULL;
	zlog_info(g_dma->log_handler,"close_dma() \n");
}

/**@defgroup Csi csi_process_module.
* @{
* @ingroup csi module
* @brief 包括记录外部用户控制csi开关. \n
* 处理IQ数据，生成频谱数据和时域数据 \n
* 发送信道估计IQ处理数据到前端接口 \n
* 保存csi原始文件写线程与主线程交互流程 
*/
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
		zlog_info(g_dma->log_handler,"rc = %d , stop_csi() \n" , rc);
		return rc;
	}
}

int start_csi_state_external(int connfd, g_dma_para* g_dma){
	int state = -1;
	if(g_dma->csi_module.user_cnt == 0 && g_dma->csi_module.csi_state == 0){
		zlog_info(g_dma->log_handler,"start csi in start_csi() \n");
		state = start_csi(g_dma);
		if(state !=0 )
			return -1;
	}

	/* need check if this connfd has in list? --------------------------------------------------------------- Note*/
	csi_user_node* tmp_node = findCsiUserNode(connfd, g_dma);
	if(tmp_node == NULL){
		csi_user_node* new_node = (csi_user_node*)malloc(sizeof(csi_user_node));
		new_node->connfd = connfd;
		new_node->g_dma = g_dma;

		list_add_tail(&new_node->list, &g_dma->csi_user_node_head);
		g_dma->csi_module.user_cnt++;
		tmp_node = new_node;
		zlog_info(g_dma->log_handler, "new csi user node ... 0x%x \n",tmp_node);
		zlog_info(g_dma->log_handler,"start csi user_cnt = %d \n",g_dma->csi_module.user_cnt);
	}

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
		// free pnode
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

	struct csi_user_node *pnode = NULL;
	list_for_each_entry(pnode, &g_dma->csi_user_node_head, list) {
		g_receive_para* tmp_receive = findReceiveNode(pnode->connfd,g_dma->g_server);
		if(tmp_receive != NULL){
			//zlog_info(g_dma->log_handler, "send csi to node js , json len : %d \n", strlen(csi_data_response_json));
			assemble_frame_and_send(tmp_receive,csi_data_response_json,strlen(csi_data_response_json),TYPE_CSI_DATA_RESPONSE);
		}
	}

	free(csi_data_response_json);	

}

/* --------------------------- csi save --------------------------------------- */

void* csi_write_thread(void* args){

	//pthread_detach(pthread_self());

	csi_save_user_node* tmp_node = (csi_save_user_node*)args;

	g_dma_para* g_dma = tmp_node->g_dma;

	zlog_info(g_dma->log_handler,"start csi_write_thread()\n");

	while(1){
		queue_item *work_item = tiny_queue_pop(tmp_node->csi_file_t->queue); // need work length

		if(work_item->buf_len == 0){
			free(work_item);
			break;
		}

		char* work = work_item->buf;
		fwrite(work,sizeof(char), work_item->buf_len, tmp_node->csi_file_t->file);
		free(work);
		free(work_item);
	}

	postMsg(MSG_CLEAR_CSI_WRITE_STATUS,NULL,0,tmp_node,0, g_dma->g_msg_queue);
	zlog_info(g_dma->log_handler,"end Exit csi_write_thread()\n");
}

int process_csi_save_file(int connfd, char* stat_buf, int stat_buf_len, g_dma_para* g_dma){
	cJSON * root = NULL;
    cJSON * item = NULL;
    root = cJSON_Parse(stat_buf);
    item = cJSON_GetObjectItem(root,"type");
	if(item->valueint != TYPE_CONTROL_SAVE_CSI){
		cJSON_Delete(root);
		return -1;
	}

	zlog_info(g_dma->log_handler, "csi save buf : %s", stat_buf);

	/* need check if this connfd has in list? --------------------------------------------------------------- Note*/
	csi_save_user_node* tmp_node = findCsiSaveUserNode(connfd, g_dma);
	//zlog_error(g_dma->log_handler, "find tmp_node : 0x%x , connfd = %d , save_user_cnt = %d \n", tmp_node, connfd, g_dma->csi_module.save_user_cnt);
	if(tmp_node == NULL){
		csi_save_user_node* new_node = (csi_save_user_node*)malloc(sizeof(csi_save_user_node));
		new_node->connfd = connfd;
		new_node->g_dma = g_dma;
		new_node->csi_file_t         = (write_file_t*)malloc(sizeof(write_file_t));
		pthread_mutex_init(&(new_node->csi_file_t->mutex),NULL);
		new_node->csi_file_t->enable = 0;
		new_node->csi_file_t->file   = NULL;
		new_node->csi_file_t->queue  = NULL;
		list_add_tail(&new_node->list, &g_dma->csi_save_user_node_head);
		g_dma->csi_module.save_user_cnt++;
		tmp_node = new_node;
		zlog_info(g_dma->log_handler, "new csi save node ... 0x%x \n",tmp_node);
	}

	item = cJSON_GetObjectItem(root,"op_cmd");
	if(item->valueint == 0){ /* stop save */
		if(tmp_node->csi_file_t->enable == 0){
			zlog_error(g_dma->log_handler,"has already stop csi save in this page : %d \n", connfd);
			cJSON_Delete(root);
			return 0;
		}
		//need inform stop
		inform_stop_csi_write_thread(connfd, g_dma);
	}else if(item->valueint == 1){ /* start save */
		if(tmp_node->csi_file_t->enable == 1){
			zlog_error(g_dma->log_handler,"has already start csi save in this page : %d \n", connfd);
			cJSON_Delete(root);
			return 0;
		}

		item = cJSON_GetObjectItem(root,"file_name");

		sprintf(tmp_node->csi_file_t->file_name, "/run/media/mmcblk1p1/gw_web/web/log/%s", item->valuestring);
		printf("csi file_name : %s\n",tmp_node->csi_file_t->file_name);
		tmp_node->csi_file_t->file = fopen(tmp_node->csi_file_t->file_name,"wb");
		if(tmp_node->csi_file_t->file == NULL){
			zlog_error(g_dma->log_handler,"Cannot create the csi file\n");
			cJSON_Delete(root);
			return -1;
		}

		tmp_node->csi_file_t->queue  = tiny_queue_create();
		if (tmp_node->csi_file_t->queue == NULL) {
			zlog_error(g_dma->log_handler,"Cannot create the csi queue\n");
			cJSON_Delete(root);
			return -1;
		}

		AddWorker(csi_write_thread,(void*)(tmp_node),g_dma->g_server->g_threadpool);

		tmp_node->csi_file_t->enable = 1;
	}

	cJSON_Delete(root);
	return 0;
}

void send_csi_to_save(g_dma_para* g_dma){
	struct csi_save_user_node *pnode = NULL;
	list_for_each_entry(pnode, &g_dma->csi_save_user_node_head, list) {
		if(pnode->csi_file_t != NULL){
			if(pnode->csi_file_t->enable == 0){
				break;
			}else if(pnode->csi_file_t->enable == 1){ /* for write file */
				char* csi_buf = malloc(1024);
				memcpy(csi_buf, g_dma->csi_spectrum->buf, 1024);
				queue_item* item = (queue_item*)malloc(sizeof(queue_item));
				item->buf = csi_buf;
				item->buf_len = 1024;	

				if (tiny_queue_push(pnode->csi_file_t->queue, item) != 0) {
					zlog_error(g_dma->log_handler,"Cannot push an element in the queue\n");
					free(csi_buf);
				}

			}
		}
	}
}

void clear_csi_write_status(csi_save_user_node* user_node, g_dma_para* g_dma){

	struct list_head *pos, *n;
    struct csi_save_user_node *pnode = NULL;
    list_for_each_safe(pos, n, &g_dma->csi_save_user_node_head){
        pnode = list_entry(pos, struct csi_save_user_node, list);
        if(pnode == user_node){
            list_del(pos);
            g_dma->csi_module.save_user_cnt--;
			zlog_info(g_dma->log_handler,"find node in clear_csi_write_status() ! save_user_cnt = %d ", g_dma->csi_module.save_user_cnt);
            break;
        }
    }

	/* close file */
	fclose(user_node->csi_file_t->file);
	user_node->csi_file_t->file = NULL;

	/* destroy queue */
	tiny_queue_destory(user_node->csi_file_t->queue);

	free(user_node->csi_file_t);
	user_node->csi_file_t = NULL;

	zlog_info(g_dma->log_handler," clear_csi_write_status :  confirm free this csi save node 0x%x \n", user_node);
	free(user_node);
}

void inform_stop_csi_write_thread(int connfd, g_dma_para* g_dma){
	csi_save_user_node* pnode = findCsiSaveUserNode(connfd, g_dma);
	if(pnode == NULL){
		zlog_error(g_dma->log_handler, "inform_stop_csi_write_thread , No user node find");
		return;
	}

	queue_item* item = (queue_item*)malloc(sizeof(queue_item));
	item->buf = NULL;
	item->buf_len = 0;	

	if (tiny_queue_push(pnode->csi_file_t->queue, item) != 0) {
		zlog_error(g_dma->log_handler,"Cannot push an 0 length element in the queue\n");
	}
	pnode->csi_file_t->enable = 0;
}
/** @} Csi*/

/* --------------------------- constellation control func --------------------------------------- */
// internel
/**@defgroup Constell constell_process_module.
* @{
* @ingroup constell module
* @brief 包括记录外部用户控制星座图开关. \n
* 处理IQ数据，生成发送前端交互格式数据 \n
* 发送数据前端接口
*/
int start_constellation(g_dma_para* g_dma){
	int rc;
	if(g_dma->p_axidma == NULL){
		zlog_info(g_dma->log_handler,"g_dma->p_axidma == NULL start_constellation\n");
		g_dma->constellation_module.constellation_state = 0;
		return -1;
	}else{
		rc = axidma_chan(g_dma->p_axidma, 0x80);
		rc = axidma_start(g_dma->p_axidma);
		g_dma->constellation_module.constellation_state = 1;
		zlog_info(g_dma->log_handler,"rc = %d , start_constellation() \n" , rc);
		return rc;
	}	
}

int stop_constellation(g_dma_para* g_dma){
	int rc;
	if(g_dma->p_axidma == NULL){
		zlog_info(g_dma->log_handler,"g_dma->p_axidma == NULL stop_constellation\n");
		g_dma->constellation_module.constellation_state = 0;
		return -1;
	}else{
		rc = axidma_stop(g_dma->p_axidma);
		g_dma->constellation_module.constellation_state = 0;
		zlog_info(g_dma->log_handler,"rc = %d , stop_constellation() \n" , rc);
		return rc;
	}
}

int start_constellation_external(int connfd, g_dma_para* g_dma){
	int state;
	if(g_dma->constellation_module.user_cnt == 0 && g_dma->constellation_module.constellation_state == 0){
		zlog_info(g_dma->log_handler,"start constellation in start_constellation() \n");
		state = start_constellation(g_dma);
	}

	/* need check if this connfd has in list? --------------------------------------------------------------- Note*/
	constell_user_node* tmp_node = findConstellUserNode(connfd, g_dma);
	if(tmp_node == NULL){
		constell_user_node* new_node = (constell_user_node*)malloc(sizeof(constell_user_node));
		new_node->connfd = connfd;
		new_node->g_dma = g_dma;

		list_add_tail(&new_node->list, &g_dma->constell_user_node_head);
		g_dma->constellation_module.user_cnt++;
		tmp_node = new_node;
		zlog_info(g_dma->log_handler, "new consteallation user node ... 0x%x \n",tmp_node);
		zlog_info(g_dma->log_handler,"start consteallation user_cnt = %d \n",g_dma->constellation_module.user_cnt);
	}

	return 0;
}

/* call by close webpage or manually*/
int stop_constellation_external(int connfd, g_dma_para* g_dma){
    struct list_head *pos, *n;
    struct constell_user_node *pnode = NULL;
    list_for_each_safe(pos, n, &g_dma->constell_user_node_head){
        pnode = list_entry(pos, struct constell_user_node, list);
        if(pnode->connfd == connfd){
            list_del(pos);
            g_dma->constellation_module.user_cnt--;
			zlog_info(g_dma->log_handler,"find node in stop_constellation_external() ! connfd = %d, user_cnt = %d ", connfd,g_dma->constellation_module.user_cnt);
            break;
        }
    }

	if(pnode != NULL){
		// free pnode
		zlog_info(g_dma->log_handler," stop_constellation_external : delete from list , free this node 0x%x \n", pnode);
		free(pnode);
	}

	if(g_dma->constellation_module.user_cnt == 0 && g_dma->constellation_module.constellation_state == 1){
		zlog_info(g_dma->log_handler,"stop constellation in stop_constellation() \n");
		stop_constellation(g_dma);
	}

	return 0;
}

void send_constell_display_in_event_loop(g_dma_para* g_dma){

	char* constell_data_response_json = constell_data_response(g_dma->cons_iq_pair->vectReal, g_dma->cons_iq_pair->vectImag, g_dma->cons_iq_pair->iq_cnt);

	struct constell_user_node *pnode = NULL;
	list_for_each_entry(pnode, &g_dma->constell_user_node_head, list) {
		g_receive_para* tmp_receive = findReceiveNode(pnode->connfd,g_dma->g_server);
		if(tmp_receive != NULL){
			//zlog_info(g_dma->log_handler, "send constell iq to node js , json len : %d , iq_cnt = %d \n", strlen(constell_data_response_json), g_dma->cons_iq_pair->iq_cnt);
			assemble_frame_and_send(tmp_receive,constell_data_response_json,strlen(constell_data_response_json),TYPE_CONSTELLATION_DATA_RESPONSE + 1);
		}
	}
	g_dma->cons_iq_pair->iq_cnt = 0;
	free(constell_data_response_json);	
}
/** @} Constell*/

/* ---------------------- CSI function ---------------------------- */
// buf_len == 1024 must /* all compare with before log data*/
/**@defgroup Csi csi_process_module.
* @{
	*/
void processCSI(char* buf, int buf_len, g_dma_para* g_dma){

	memcpy(g_dma->csi_spectrum->buf, buf, buf_len); // buf_len = 1024

    parse_IQ_from_net(buf, buf_len, g_dma->csi_spectrum->in_IQ);
    calculate_spectrum(g_dma->csi_spectrum->in_IQ, g_dma->csi_spectrum->out_fft, &(g_dma->csi_spectrum->p), g_dma->csi_spectrum->spectrum, 256); 
    myfftshift(g_dma->csi_spectrum->db_array, g_dma->csi_spectrum->spectrum, 256);
    timeDomainChange(g_dma->csi_spectrum->in_IQ, g_dma->csi_spectrum->time_IQ, 256);
}

/** @} Csi*/

/* ---------------------- process constellation function ---------------------------- */
/**@defgroup Constell constell_process_module.
* @{
	*/
int processConstellation(char* buf, int buf_len, g_dma_para* g_dma){
	// g_dma->cons_iq_pair->iq_cnt = 0;

	// find I data --- 0 : I , 1 : Q 
	int start_idx = 0;
	int end_idx = buf_len;
	//zlog_info(g_dma->log_handler,"checkIQ(buf[start_idx] = %d, buf[start_idx] = %d , buf[start_idx+1 = %d ",checkIQ(buf[start_idx]), buf[start_idx],buf[start_idx+1]);
	if(checkIQ(buf[start_idx]) == 1){
		start_idx = start_idx + 1;
		if(checkIQ(buf[start_idx]) == 1){
			zlog_error(g_dma->log_handler,"Error in IQ sequence \n ");
			return -1;
		}
	}
	if((buf_len - start_idx)%2 == 1)
		end_idx = buf_len - 1;
	
	int i=0;
	int iq_cnt = g_dma->cons_iq_pair->iq_cnt;
	int sum_cnt = 0;
	int sw_temp = 0;
	for(i=start_idx;i<end_idx;i++){
		//zlog_info(g_dma->log_handler, "buf[i] = %d", buf[i]);
		buf[i] = buf[i] << 1; // shift to 8 bit, low bit fill 0, -128 - 127
		//zlog_info(g_dma->log_handler, "buf[i]<<1 = %d ", buf[i]);
		unsigned short tmp = (unsigned short)(buf[i]);
		int value_i = tmp;
		if(value_i > 127)
			value_i = value_i - 256;
		if(sw_temp == 0){
			g_dma->cons_iq_pair->vectReal[iq_cnt] = value_i;
			sw_temp = 1;
		}else if(sw_temp == 1){
			g_dma->cons_iq_pair->vectImag[iq_cnt] = value_i;
			sw_temp = 0;
			iq_cnt = iq_cnt + 1;
			sum_cnt = sum_cnt + 1;
		}
	}
	g_dma->cons_iq_pair->iq_cnt = g_dma->cons_iq_pair->iq_cnt + sum_cnt;
	if(g_dma->cons_iq_pair->iq_cnt > 1500){
		return 0;
	}else{
		return 1;
	}
}
/** @} Constell*/