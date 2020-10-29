#ifndef WEB_COMMON_H
#define WEB_COMMON_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <assert.h>
#include "tiny_queue.h"

// hardware version 
// #define HARD_VERSION_RFV2

#define COPY_BUFFER     1
#define NOT_COPY_BUFFER 0

/* write file */
/**@struct write_file_t
* @brief 定义存储文件相关资源
*/
typedef struct write_file_t{
	pthread_mutex_t  	mutex;
	int             	enable; // produce_enable: 
	tiny_queue_t*  		queue;
	char           		file_name[1024];
	FILE*          		file;
}write_file_t;

/**@struct queue_item
* @brief 定义队列元素
*/
typedef struct queue_item{
	char* buf;
	int   buf_len;
}queue_item;

/**@enum msg_event
* @brief 定义上报事件循环处理消息类型
*/
typedef enum msg_event{
	/* test msg type */
    MSG_NETWORK = 1,
	MSG_AIR,
	MSG_MONITOR,
	MSG_TIMEOUT,
	MSG_TIMEOUT_TEST,
	MSG_CONF_CHANGE,
	MSG_OPENWRT_CONNECTED,
	MSG_PRIORITY_KEEPALIVE,
	/* process new client */
	MSG_ACCEPT_NEW_USER,
	MSG_DEL_DISCONNECT_USER,
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
	/* csi and constellation */
	MSG_START_CSI,
	MSG_STOP_CSI,
	MSG_CSI_READY,
	MSG_START_CONSTELLATION,
	MSG_STOP_CONSTELLATION,
	MSG_CONSTELLATION_READY,
	MSG_CONTROL_SAVE_IQ_DATA,
	MSG_CLEAR_CSI_WRITE_STATUS,
	/* cmd distance app */
	MSG_OPEN_DISTANCE_APP,
	MSG_CLOSE_DISTANCE_APP,
	/* cmd dac */
	MSG_OPEN_DAC,
	MSG_CLOSE_DAC,
	/* clear log */
	MSG_CLEAR_LOG,
	/* new add 200103 */
	MSG_RESET_SYSTEM,
	MSG_INQUIRY_STATISTICS,
	MSG_IP_SETTING,
	/* RF state request */
	MSG_INQUIRY_RF_INFO,
	MST_RF_INFO_READY,
	MSG_RF_FREQ_SETTING,
	/* SETTING CMD */
	MSG_OPEN_TX_POWER,
	MSG_CLOSE_TX_POWER,
	MSG_OPEN_RX_GAIN,
	MSG_CLOSE_RX_GAIN,
}msg_event;

#define FRAME_HEAD_ROOM 8

typedef enum frame_type{
	TYPE_SYSTEM_STATE_REQUEST = 1,
	TYPE_SYSTEM_STATE_RESPONSE,
	TYPE_SYSTEM_STATE_EXCEPTION = 4,
	TYPE_REG_STATE_REQUEST = 31,
	TYPE_REG_STATE_RESPONSE,
	TYPE_INQUIRY_RSSI_REQUEST = 41,
	TYPE_RSSI_DATA_RESPONSE,
	TYPE_RSSI_CONTROL,
	TYPE_START_CSI = 51,
	TYPE_STOP_CSI,
	TYPE_CONTROL_SAVE_CSI,
	TYPE_CSI_DATA_RESPONSE,
	TYPE_START_CONSTELLATION = 61,
	TYPE_STOP_CONSTELLATION,
	TYPE_CONSTELLATION_DATA_RESPONSE,
	TYPE_CMD_STATE_RESPONSE = 70,
	TYPE_OPEN_DISTANCE_APP = 71,
	TYPE_CLOSE_DISTANCE_APP,
	TYPE_OPEN_DAC,
	TYPE_CLOSE_DAC,
	TYPE_CLEAR_LOG,
	// IP setting ------------- below is new type
	TYPE_IP_SETTING,
	// RESET 
	TYPE_RESET, 

	// Statistics info
	TYPE_STATISTICS_INFO = 81,
	TYPE_STATISTICS_RESPONSE,
	// RF info
	TYPE_RF_INFO = 83,
	TYPE_RF_INFO_RESPONSE,
	// RF setting
	TYPE_RF_FREQ_SETTING = 91,
	TYPE_OPEN_TX_POWER,
	TYPE_CLOSE_TX_POWER,
	TYPE_OPEN_RX_GAIN,
	TYPE_CLOSE_RX_GAIN,

	TYPE_OPENWRT_KEEPALIVE = 201,
	TYPE_OPENWRT_KEEPALIVE_RESPONSE,

}frame_type;

/**@struct reg_state_t
* @brief 定义寄存器数据类型
*/
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