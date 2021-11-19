#ifndef GW_CONTROL_H
#define GW_CONTROL_H

#include "zlog.h"
#include <stdint.h>
#include "regdev_common.h"

#define	REG_MAP_SIZE	0X10000

#define	REG_C1_ADDR	    0x43c10000

#define	REG_C0_ADDR	    0x43c00000

#define SYSFS_GPIO_RST_VAL          "/sys/class/gpio/gpio973/value"
#define SYSFS_GPIO_RST_VAL_H        "1"
#define SYSFS_GPIO_RST_VAL_L        "0"


typedef struct g_RegDev_para{
	//void*  			   mem_dev_phy;
	struct mem_map_s*  mem_dev_phy; // c2
	zlog_category_t*   log_handler;
}g_RegDev_para;

int gpio_read(int pin);

 /* ----------------------- new interface to read and write register ---------------------------- */
int initRegdev(g_RegDev_para** g_RegDev, uint32_t reg_phy_addr, zlog_category_t* handler);
int set_dst_mac_fast(g_RegDev_para* g_RegDev, char* dst_mac_buf);
int set_src_mac_fast(g_RegDev_para* g_RegDev, char* src_mac_buf);

/* ----------------------------- process ddr_tx_hold_on and mac_id sync ---------------------------- */
int open_ddr(g_RegDev_para* g_RegDev);
int close_ddr(g_RegDev_para* g_RegDev);
int trigger_mac_id(g_RegDev_para* g_RegDev);

/* ----------------------------- get power , crc correct cnt , crc error cnt ---------------------------- */
uint32_t getPowerLatch(g_RegDev_para* g_RegDev);
uint32_t get_crc_correct_cnt(g_RegDev_para* g_RegDev);
uint32_t get_crc_error_cnt(g_RegDev_para* g_RegDev);

/* ----------------------------- get airdata_buf2_empty_flag , airsignal_buf2_empty_flag-------------------------- */
uint32_t ddr_empty_flag(g_RegDev_para* g_RegDev);
uint32_t airdata_buf2_empty_flag(g_RegDev_para* g_RegDev);
uint32_t airsignal_buf2_empty_flag(g_RegDev_para* g_RegDev);

uint32_t read_unfilter_byte_low32(g_RegDev_para* g_RegDev);
uint32_t read_unfilter_byte_high32(g_RegDev_para* g_RegDev);
uint32_t rx_byte_filter_ether_low32(g_RegDev_para* g_RegDev);
uint32_t rx_byte_filter_ether_high32(g_RegDev_para* g_RegDev);


/* ------------------------------ reset bb ------------------------------------ */
int reset_bb(g_RegDev_para* g_RegDev);
int release_bb(g_RegDev_para* g_RegDev);

/* ----------------------------- ready handover related (snr , rx mcs ,) ---------------------------- */
double get_rx_snr(g_RegDev_para* g_RegDev);
uint32_t get_rx_mcs(g_RegDev_para* g_RegDev);

/* ----------------------------- distance measure --------------------------------------------------- */
uint32_t get_delay_tick(g_RegDev_para* g_RegDev);
int set_delay_tick(g_RegDev_para* g_RegDev, uint32_t delay);

/* ----------------------------- webapp use ----------------------------------------------------------*/
// getPowerLatch -- 0x124
uint32_t get_fpga_version(g_RegDev_para* g_RegDev);
uint32_t get_sync_failed_stastic(g_RegDev_para* g_RegDev);
uint32_t get_freq_offset(g_RegDev_para* g_RegDev);
uint32_t get_pac_txc_misc(g_RegDev_para* g_RegDev);
uint32_t get_pac_txc_re_trans_cnt(g_RegDev_para* g_RegDev);
uint32_t get_pac_txc_expect_seq_id(g_RegDev_para* g_RegDev);
uint32_t get_rxc_miscs(g_RegDev_para* g_RegDev);
uint32_t get_rx_sync(g_RegDev_para* g_RegDev);
uint32_t get_ctrl_frame_crc_correct_cnt(g_RegDev_para* g_RegDev);
uint32_t get_ctrl_frame_crc_error_cnt(g_RegDev_para* g_RegDev);
uint32_t get_manage_frame_crc_correct_cnt(g_RegDev_para* g_RegDev);
uint32_t get_manage_frame_crc_error_cnt(g_RegDev_para* g_RegDev);
uint32_t get_bb_send_cnt(g_RegDev_para* g_RegDev);
uint32_t get_rx_vector(g_RegDev_para* g_RegDev);
uint32_t get_pac_soft_rst(g_RegDev_para* g_RegDev);
uint32_t get_sw_fifo_data_cnt(g_RegDev_para* g_RegDev);
// snr -- 0x140
uint32_t get_delay_RW(g_RegDev_para* g_RegDev);
// get_delay_tick -- 0x864


/// tmp add for uestc project
uint32_t get_evm(g_RegDev_para* g_RegDev);
uint32_t get_ber(g_RegDev_para* g_RegDev);

#endif//GW_CONTROL_H
