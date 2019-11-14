#ifndef WEB_COMMON_H
#define WEB_COMMON_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef enum msg_event{
	/* test msg type */
    MSG_NETWORK = 1,
	MSG_AIR,
	MSG_MONITOR,
	MSG_TIMEOUT,
	/* process new client */
	MSG_ACCEPT_NEW_USER,
	MSG_RECEIVE_THREAD_CLOSED,
	/* reg state request */
	MSG_INQUIRY_REG_STATE,
	/* RF state request */
	MSG_INQUIRY_RF_MF_STATE,
}msg_event;

// web system state 

// #define STATE_STARTUP           0
// #define STATE_WAIT_MONITOR      1
// #define STATE_INIT_SELECTED     2
// #define STATE_WORKING           3
// #define STATE_TARGET_SELECTED   4




#endif//WEB_COMMON_H