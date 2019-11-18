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
	/* system state */
	MSG_INQUIRY_SYSTEM_STATE,
	MSG_SYSTEM_STATE_EXCEPTION,
	/* reg state request */
	MSG_INQUIRY_REG_STATE,
	/* RSSI control and value */
	MSG_INQUIRY_RSSI,
	MSG_CONTROL_RSSI,
	/* RF state request */
	MSG_INQUIRY_RF_MF_STATE,
}msg_event;

// web system state 

// #define STATE_STARTUP           0
// #define STATE_WAIT_MONITOR      1
// #define STATE_INIT_SELECTED     2
// #define STATE_WORKING           3
// #define STATE_TARGET_SELECTED   4

#define FRAME_HEAD_ROOM 8

typedef enum frame_type{
	TYPE_SYSTEM_STATE_REQUEST = 1,
	TYPE_SYSTEM_STATE_RESPONSE,
	TYPE_SYSTEM_STATE_EXCEPTION,
	TYPE_REG_STATE_REQUEST = 31,
	TYPE_REG_STATE_RESPONSE,
	TYPE_INQUIRY_RSSI_REQUEST = 41,
	TYPE_RSSI_RESPONSE,
	TYPE_RSSI_CONTROL,
}frame_type;

typedef struct reg_state_t{
	int             reg_state_num;
	unsigned int    power_est_latch;
	unsigned int    coarse_low16;
	unsigned int    fine_high16;
	double          freq_offset;
	int             tx_modulation;
	int             dac_state;
	double          snr;
	double          distance;
}reg_state_t;




#endif//WEB_COMMON_H