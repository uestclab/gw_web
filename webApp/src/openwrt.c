
#include "openwrt.h"

/* openwrt keepAlive check */
typedef struct openwrt_keepAlive_t{
    g_server_para* g_server;
    int connfd;
}openwrt_keepAlive_t;

void setOpenwrtState(g_server_para* g_server){
    pthread_mutex_lock(&(g_server->openwrt_node.mutex));
    g_server->openwrt_node.openwrt_link = 1;
    pthread_mutex_unlock(&(g_server->openwrt_node.mutex));
}

void openwrt_check_keepAlive(ngx_event_t *ev){
    openwrt_keepAlive_t* tmp_priv = (openwrt_keepAlive_t*)(ev->data);
    int link = 0;
    {
        pthread_mutex_lock(&(tmp_priv->g_server->openwrt_node.mutex));
        link = tmp_priv->g_server->openwrt_node.openwrt_link;
        tmp_priv->g_server->openwrt_node.openwrt_link = 0;
        pthread_mutex_unlock(&(tmp_priv->g_server->openwrt_node.mutex));
    }
    if(!link){
        postMsg(MSG_DEL_DISCONNECT_USER,NULL,0,NULL,tmp_priv->connfd,tmp_priv->g_server->g_msg_queue);
    }else{
        // openwrt_start_keepAlive(tmp_priv->g_server, tmp_priv->connfd); // sent keepAlive again.... 
        postMsg(MSG_PRIORITY_KEEPALIVE,NULL,0,NULL,tmp_priv->connfd,tmp_priv->g_server->g_msg_queue);
    }
    free(tmp_priv);
    free(ev);
}

void openwrt_start_keepAlive(g_server_para* g_server, int connfd){
    openwrt_keepAlive_t* tmp_priv = (openwrt_keepAlive_t*)malloc(sizeof(openwrt_keepAlive_t));
    tmp_priv->g_server = g_server;
    tmp_priv->connfd   = connfd;

    g_receive_para* tmp_receive = findReceiveNode(connfd,g_server); 
    if(tmp_receive == NULL){
        zlog_error(g_server->log_handler, " openwrt_start_keepAlive stop !");
        return;
    }

    assemble_frame_and_send(tmp_receive, NULL, 0, TYPE_OPENWRT_KEEPALIVE);
    addTimerTaskInterface((void*)tmp_priv, openwrt_check_keepAlive, 1000, g_server->g_timer);
}