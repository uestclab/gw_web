#ifndef WEB_COMMON_H
#define WEB_COMMON_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <assert.h>

typedef enum msg_event{
	/* test msg type */
    MSG_NETWORK = 1,
	MSG_AIR,
	MSG_MONITOR,
	MSG_TIMEOUT,
	MSG_CONF_CHANGE,
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
	MSG_RSSI_READY_AND_SEND,
	MSG_CONTROL_RSSI,
	MSG_CLEAR_RSSI_WRITE_STATUS,
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
	TYPE_RSSI_DATA_RESPONSE,
	TYPE_RSSI_CONTROL,
}frame_type;

typedef struct reg_state_t{
	int             reg_state_num;
	unsigned int    power_est_latch;
	unsigned int    coarse_low16;
	unsigned int    fine_high16;
	double          freq_offset;
	int             tx_modulation;
	unsigned int    txc_retrans_cnt;
	unsigned int    expect_seq_id_low16;
	unsigned int    expect_seq_id_high16;
	unsigned int    rx_id;
	unsigned int    rx_fifo_data;
	unsigned int    rx_sync;
	unsigned int    rx_v_agg_num;
	unsigned int    rx_v_len;
	unsigned int    tx_abort;
	unsigned int    ddr_closed;
	unsigned int    sw_fifo_cnt;
	unsigned int    bb_send_cnt;
	unsigned int    ctrl_crc_c;
	unsigned int    ctrl_crc_e;
	unsigned int    manage_crc_c;
	unsigned int    manage_crc_e;
	int             dac_state;
	double          snr;
	double          distance;
}reg_state_t;




#endif//WEB_COMMON_H