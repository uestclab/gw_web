#ifndef _OPENWRT_H
#define _OPENWRT_H

#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/syscall.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/epoll.h>

#include "zlog.h"
#include "server.h"
#include "msg_queue.h"
#include "cJSON.h"
#include "web_common.h"
#include "gw_macros_util.h"
#include "small_utility.h"
#include "utility.h"
#include "timer.h"

void setOpenwrtState(g_server_para* g_server);
void openwrt_start_keepAlive(g_server_para* g_server, int connfd);


#endif//_OPENWRT_H