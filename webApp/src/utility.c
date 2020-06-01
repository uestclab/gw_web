#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <linux/if.h>
#include <linux/sockios.h>
#include <linux/ethtool.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/shm.h>

// -1 -- error , details can check errno
// 1 -- interface link up
// 0 -- interface link down.
int get_netlink_status(const char *if_name)
{
    int skfd,ret;
    struct ifreq ifr;
    struct ethtool_value edata;
    edata.cmd = ETHTOOL_GLINK;
    edata.data = 0;
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, if_name, sizeof(ifr.ifr_name) - 1);
    ifr.ifr_data = (char *) &edata;
    if (( skfd = socket( AF_INET, SOCK_DGRAM, 0 )) < 0){
        return -1;
    }
    if(ioctl( skfd, SIOCETHTOOL, &ifr ) < 0){
        close(skfd);
        return ret;
    }
    close(skfd);
    return edata.data;
}

int connect_helloworld(){

    int sock_cli = socket(AF_INET,SOCK_STREAM, 0);
    int server_port = 55012;
    char* server_ip = "192.168.0.1";
    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(server_port);
    servaddr.sin_addr.s_addr = inet_addr(server_ip);

	int try_cnt = 3;
    int connect_stat = connect(sock_cli, (struct sockaddr *)&servaddr, sizeof(servaddr));
    if(connect_stat >= 0){
        return sock_cli;
    }
    while (try_cnt > 0){
        try_cnt = try_cnt - 1;	
		connect_stat = connect(sock_cli, (struct sockaddr *)&servaddr, sizeof(servaddr));
        if(connect_stat >= 0){
            return sock_cli;
        }
    }
    return -1;
}