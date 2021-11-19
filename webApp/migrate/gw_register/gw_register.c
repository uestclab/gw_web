#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "gw_utility.h"
#include "gw_register.h"

#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>


 /* ----------------------- new interface to read and write register ---------------------------- */

int initRegdev(g_RegDev_para** g_RegDev, uint32_t reg_phy_addr, zlog_category_t* handler)
{
	zlog_info(handler,"initPhyRegdev()");

	*g_RegDev = (g_RegDev_para*)malloc(sizeof(struct g_RegDev_para));

	(*g_RegDev)->mem_dev_phy = NULL;
	(*g_RegDev)->log_handler = handler;


	int rc = 0;
    // #define	REG_PHY_ADDR	0x43C20000
	regdev_init(&((*g_RegDev)->mem_dev_phy));
	regdev_set_para((*g_RegDev)->mem_dev_phy, reg_phy_addr, REG_MAP_SIZE);
	rc = regdev_open((*g_RegDev)->mem_dev_phy);
	if(rc < 0){
		zlog_info(handler," mem_dev_phy regdev_open err !!\n");
		return -1;
	}


	return 0;
}

int gpio_read(int pin)
{  
    char path[64];  

    char value_str[3];  

    int fd;  

  
	/* /sys/class/gpio/gpio973/value */
    snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d/value", pin);  

    fd = open(path, O_RDONLY);  

    if (fd < 0){
        printf("Failed to open gpio value for reading!\n");
        return -1;
    }  

    if (read(fd, value_str, 3) < 0){
        printf("Failed to read value!\n");
        return -1;
    }
    close(fd);
    return (atoi(value_str));
}

/* ------------------------------------------------------ */

int set_dst_mac_fast(g_RegDev_para* g_RegDev, char* dst_mac_buf){ // mac_buf[6]
	zlog_info(g_RegDev->log_handler,"set_dst_mac_fast\n");

	uint32_t low32 = getLow32(dst_mac_buf);
	uint32_t high16 = getHigh16(dst_mac_buf);	
	
	// dest_mac_0_low_32_bit
	int	rc = regdev_write(g_RegDev->mem_dev_phy, 0x838, low32);
	if(rc < 0){
		zlog_info(g_RegDev->log_handler,"dest_mac_0_low_32_bit write failed !!! \n");
		return rc;
	}
	//dest_mac_1_high_16_bit
	rc = regdev_write(g_RegDev->mem_dev_phy, 0x83c, high16);
	if(rc < 0){
		zlog_info(g_RegDev->log_handler,"dest_mac_1_high_16_bit write failed !!! \n");
		return rc;
	}
	return 0;
}

int set_src_mac_fast(g_RegDev_para* g_RegDev, char* src_mac_buf){ //src_mac_buf[6]
	zlog_info(g_RegDev->log_handler,"set_src_mac_fast\n");

	uint32_t low32 = getLow32(src_mac_buf);
	uint32_t high16 = getHigh16(src_mac_buf);

	// src_mac_0_low_32_bit
	int	rc = regdev_write(g_RegDev->mem_dev_phy, 0x830, low32);
	if(rc < 0){
		zlog_info(g_RegDev->log_handler,"src_mac_0_low_32_bit write failed !!! \n");
		return rc;
	}
	//src_mac_1_high_16_bit
	rc = regdev_write(g_RegDev->mem_dev_phy, 0x834, high16);
	if(rc < 0){
		zlog_info(g_RegDev->log_handler,"src_mac_1_high_16_bit write failed !!! \n");
		return rc;
	}
	return 0;
}

/* ----------------------------- process ddr_tx_hold_on and mac_id sync ---------------------------- */

// bit : 2 , 0 -- open , 1 -- close
int open_ddr(g_RegDev_para* g_RegDev){
	zlog_info(g_RegDev->log_handler,"open_ddr\n");

	uint32_t value = 0x00000000;
	int	stat = regdev_read(g_RegDev->mem_dev_phy, 0x82c, &value);

	value = value & (~(0x1<<2));
	value = value | (0x0<<2);
	//value = value << 24;
	int	rc = regdev_write(g_RegDev->mem_dev_phy, 0x82c, value); // Note :
	if(rc < 0){
		zlog_info(g_RegDev->log_handler,"open_ddr write failed !!! \n");
		return rc;
	}
	return 0;
}

int close_ddr(g_RegDev_para* g_RegDev){
	zlog_info(g_RegDev->log_handler,"close_ddr\n");

	uint32_t value = 0x00000000;
	int	stat = regdev_read(g_RegDev->mem_dev_phy, 0x82c, &value);

	value = value & (~(0x1<<2));
	value = value | (0x1<<2);
	int	rc = regdev_write(g_RegDev->mem_dev_phy, 0x82c, value); // Note :
	if(rc < 0){
		zlog_info(g_RegDev->log_handler,"close_ddr write failed !!! \n");
		return rc;
	}
	return 0;
}

// bit : 1 , 1 -- trigger , fpga set 0 
int trigger_mac_id(g_RegDev_para* g_RegDev){
	zlog_info(g_RegDev->log_handler,"trigger_mac_id\n");

	uint32_t value = 0x00000000;
	int	stat = regdev_read(g_RegDev->mem_dev_phy, 0x82c, &value);

	value = value & (~(0x1<<1));
	value = value | (0x1<<1);
	int	rc = regdev_write(g_RegDev->mem_dev_phy, 0x82c, value); // Note :
	if(rc < 0){
		zlog_info(g_RegDev->log_handler,"trigger_mac_id write failed !!! \n");
		return rc;
	}
	return 0;
}

/* ----------------------------- get ddr_full_flag , airdata_buf2_empty_flag , airsignal_buf2_empty_flag ---------------------------- */



uint32_t ddr_empty_flag(g_RegDev_para* g_RegDev){ // value != 0 : full
	uint32_t flag = 0x00000000;
	int	rc = regdev_read(g_RegDev->mem_dev_phy, 0x82c, &flag);
	if(rc < 0){
		zlog_info(g_RegDev->log_handler,"ddr_empty_flag failed !!! \n");
		return rc;
	}

	// bit3 ~ bit4
	uint32_t value = flag & (0x3<<3);
	value = value>>3;

	return value;
}

uint32_t airdata_buf2_empty_flag(g_RegDev_para* g_RegDev){
	//zlog_info(g_RegDev->log_handler,"airdata_buf2_empty_flag\n");
	uint32_t fifo_data_cnt_read = 0x00000000;
	int	rc = regdev_read(g_RegDev->mem_dev_phy, 0x81c, &fifo_data_cnt_read);
	if(rc < 0){
		zlog_info(g_RegDev->log_handler,"fifo_data_cnt_read failed !!! \n");
		return rc;
	}

	// bit0 ~ bit4
	uint32_t fifo_data_cnt = fifo_data_cnt_read & (0x1f);

	zlog_info(g_RegDev->log_handler,"fifo_data_cnt = %u \n",fifo_data_cnt);

// ------------------------------------------------------------

	uint32_t tx_win_cnt_read = 0x00000000;
	int	rc_1 = regdev_read(g_RegDev->mem_dev_phy, 0x80c, &tx_win_cnt_read);
	if(rc_1 < 0){
		zlog_info(g_RegDev->log_handler,"tx_win_cnt_read failed !!! \n");
		return rc_1;
	}

	// bit8 ~ bit10
	uint32_t tx_win_cnt = tx_win_cnt_read & (0x7<<8);
	tx_win_cnt = tx_win_cnt>>8;

	zlog_info(g_RegDev->log_handler,"tx_win_cnt = %u \n",tx_win_cnt);

	if(fifo_data_cnt == 0 && tx_win_cnt == 0)
		return 1;
	else
		return 0;
}

uint32_t airsignal_buf2_empty_flag(g_RegDev_para* g_RegDev){
	//zlog_info(g_RegDev->log_handler,"airsignal_buf2_empty_flag\n");
	uint32_t sw_fifo_data_cnt_read = 0x00000000;
	int	rc = regdev_read(g_RegDev->mem_dev_phy, 0x81c, &sw_fifo_data_cnt_read);
	if(rc < 0){
		zlog_info(g_RegDev->log_handler,"sw_fifo_data_cnt_read failed !!! \n");
		return rc;
	}

	// bit7 ~ bit9
	uint32_t sw_fifo_data_cnt = sw_fifo_data_cnt_read & (0x7<<7);
	sw_fifo_data_cnt = sw_fifo_data_cnt>>7;

	zlog_info(g_RegDev->log_handler,"sw_fifo_data_cnt = %u \n",sw_fifo_data_cnt);

// ------------------------------------------------------------

	uint32_t sw_tx_win_cnt_read = 0x00000000;
	int	rc_1 = regdev_read(g_RegDev->mem_dev_phy, 0x80c, &sw_tx_win_cnt_read);
	if(rc_1 < 0){
		zlog_info(g_RegDev->log_handler,"sw_tx_win_cnt_read failed !!! \n");
		return rc_1;
	}

	// bit20 ~ bit21
	uint32_t sw_tx_win_cnt = sw_tx_win_cnt_read & (0x3<<20);
	sw_tx_win_cnt = sw_tx_win_cnt>>20;

	zlog_info(g_RegDev->log_handler,"sw_tx_win_cnt = %u \n",sw_tx_win_cnt);

	if(sw_fifo_data_cnt == 0 && sw_tx_win_cnt == 0)
		return 1;
	else
		return 0;
}



/* ----------------------------- get power , crc correct cnt , crc error cnt ---------------------------- */

uint32_t getPowerLatch(g_RegDev_para* g_RegDev){
	//zlog_info(g_RegDev->log_handler,"getPowerLatch\n");
	uint32_t power = 0x00000000;
	int	rc = regdev_read(g_RegDev->mem_dev_phy, 0x124, &power);
	if(rc < 0){
		zlog_info(g_RegDev->log_handler,"getPowerLatch failed !!! \n");
		return rc;
	}
	return power;
}

uint32_t get_crc_correct_cnt(g_RegDev_para* g_RegDev){
	//zlog_info(g_RegDev->log_handler,"get_crc_correct_cnt\n");
	uint32_t crc_correct_cnt = 0x00000000;
	int	rc = regdev_read(g_RegDev->mem_dev_phy, 0x844, &crc_correct_cnt);
	if(rc < 0){
		zlog_info(g_RegDev->log_handler,"get_crc_correct_cnt failed !!! \n");
		return rc;
	}
	return crc_correct_cnt;
}

uint32_t get_crc_error_cnt(g_RegDev_para* g_RegDev){
	//zlog_info(g_RegDev->log_handler,"get_crc_error_cnt\n");
	uint32_t crc_error_cnt = 0x00000000;
	int	rc = regdev_read(g_RegDev->mem_dev_phy, 0x848, &crc_error_cnt);
	if(rc < 0){
		zlog_info(g_RegDev->log_handler,"get_crc_error_cnt failed !!! \n");
		return rc;
	}
	return crc_error_cnt;
}

/* ----------------------------- ready handover related (snr , rx mcs ,) ---------------------------- */
double get_rx_snr(g_RegDev_para* g_RegDev){
	uint32_t snr_i = 0x00000000;
	int	rc = regdev_read(g_RegDev->mem_dev_phy, 0x140, &snr_i);
	if(rc < 0){
		zlog_info(g_RegDev->log_handler,"get_rx_snr failed !!! \n");
		return -1.0;
	}
	double snr_tmp = snr_i / 4.0;
	return snr_tmp;
}

// 0 : bpsk , 1 : qpsk 2 , 2 : 16qam
uint32_t get_rx_mcs(g_RegDev_para* g_RegDev){
	uint32_t rx_vector = 0x0;
	int	rc = regdev_read(g_RegDev->mem_dev_phy, 0x84c, &rx_vector);
	if(rc < 0){
		zlog_info(g_RegDev->log_handler,"get_rx_mcs failed !!! \n");
		return rc;
	}
	//uint32_t rx_mcs= (rx_vector << 14) >> 14;// [17:0]
	//unsigned int agg_num = (cnt << 5) >> 28;// [26:23]
	uint32_t rx_mcs = (rx_vector<<9) >> 27;//[22:18]
	return rx_mcs;
}

// ----------------------------------------------------------------------------------------

// reset bb
//devmem 0x43c20004  32 0x2 \n
//devmem 0x43c20004  32 0x0 \n
int reset_bb(g_RegDev_para* g_RegDev){
	zlog_info(g_RegDev->log_handler,"reset_bb\n");
	uint32_t value = 0x00000002;
	int	rc = regdev_write(g_RegDev->mem_dev_phy, 0x4, value);
	if(rc < 0){
		zlog_info(g_RegDev->log_handler,"reset_bb_1 write failed !!! \n");
		return rc;
	}

	return 0;
}

int release_bb(g_RegDev_para* g_RegDev){
	zlog_info(g_RegDev->log_handler,"release_bb\n");
	uint32_t value = 0x0;
	int	rc = regdev_write(g_RegDev->mem_dev_phy, 0x4, value);
	if(rc < 0){
		zlog_info(g_RegDev->log_handler,"release_bb_1 write failed !!! \n");
		return rc;
	}
	return 0;
}


// delay tick for distance measure
uint32_t get_delay_tick(g_RegDev_para* g_RegDev){
	//zlog_info(g_RegDev->log_handler,"get_delay_tick\n");
	uint32_t delay_tick = 0x00000000;
	int	rc = regdev_read(g_RegDev->mem_dev_phy, 0x864, &delay_tick);
	if(rc < 0){
		zlog_info(g_RegDev->log_handler,"get_delay_tick failed !!! \n");
		return rc;
	}
	return delay_tick;	
}

int set_delay_tick(g_RegDev_para* g_RegDev, uint32_t delay){
	zlog_info(g_RegDev->log_handler,"set_delay_tick\n");
	int	rc = regdev_write(g_RegDev->mem_dev_phy, 0x868, delay);
	if(rc < 0){
		zlog_info(g_RegDev->log_handler,"set_delay_tick write failed !!! \n");
		return rc;
	}

	return 0;
}


/* for webapp use */
// getPowerLatch -- 0x124

// fpga_version -- 0x8
uint32_t get_fpga_version(g_RegDev_para* g_RegDev){
	uint32_t value = 0x00000000;
	int	rc = regdev_read(g_RegDev->mem_dev_phy, 0x8, &value);
	if(rc < 0){
		zlog_info(g_RegDev->log_handler,"get_fpga_version failed !!! \n");
		return rc;
	}
	return value;
}

// sync_failed_stastic -- 0x12c
uint32_t get_sync_failed_stastic(g_RegDev_para* g_RegDev){
	uint32_t value = 0x00000000;
	int	rc = regdev_read(g_RegDev->mem_dev_phy, 0x12c, &value);
	if(rc < 0){
		zlog_info(g_RegDev->log_handler,"get_sync_failed_stastic failed !!! \n");
		return rc;
	}
	return value;
}

// freq_offset -- 0x138
uint32_t get_freq_offset(g_RegDev_para* g_RegDev){
	uint32_t value = 0x00000000;
	int	rc = regdev_read(g_RegDev->mem_dev_phy, 0x138, &value);
	if(rc < 0){
		zlog_info(g_RegDev->log_handler,"get_freq_offset failed !!! \n");
		return rc;
	}
	return value;
}

// pac_txc_misc -- 0x80c
uint32_t get_pac_txc_misc(g_RegDev_para* g_RegDev){
	uint32_t value = 0x00000000;
	int	rc = regdev_read(g_RegDev->mem_dev_phy, 0x80c, &value);
	if(rc < 0){
		zlog_info(g_RegDev->log_handler,"get_pac_txc_misc failed !!! \n");
		return rc;
	}
	return value;
}

// pac_txc_re_trans_cnt -- 0x810
uint32_t get_pac_txc_re_trans_cnt(g_RegDev_para* g_RegDev){
	uint32_t value = 0x00000000;
	int	rc = regdev_read(g_RegDev->mem_dev_phy, 0x810, &value);
	if(rc < 0){
		zlog_info(g_RegDev->log_handler,"get_pac_txc_re_trans_cnt failed !!! \n");
		return rc;
	}
	return value;
}

// pac_txc_expect_seq_id -- 0x814
uint32_t get_pac_txc_expect_seq_id(g_RegDev_para* g_RegDev){
	uint32_t value = 0x00000000;
	int	rc = regdev_read(g_RegDev->mem_dev_phy, 0x814, &value);
	if(rc < 0){
		zlog_info(g_RegDev->log_handler,"get_pac_txc_expect_seq_id failed !!! \n");
		return rc;
	}
	return value;
}

// rxc_miscs -- 0x820
uint32_t get_rxc_miscs(g_RegDev_para* g_RegDev){
	uint32_t value = 0x00000000;
	int	rc = regdev_read(g_RegDev->mem_dev_phy, 0x820, &value);
	if(rc < 0){
		zlog_info(g_RegDev->log_handler,"get_rxc_miscs failed !!! \n");
		return rc;
	}
	return value;
}

// rx_sync -- 0x120
uint32_t get_rx_sync(g_RegDev_para* g_RegDev){
	uint32_t value = 0x00000000;
	int	rc = regdev_read(g_RegDev->mem_dev_phy, 0x120, &value);
	if(rc < 0){
		zlog_info(g_RegDev->log_handler,"get_rx_sync failed !!! \n");
		return rc;
	}
	return value;
}

// ctrl_frame_crc_correct_cnt -- 0x850
uint32_t get_ctrl_frame_crc_correct_cnt(g_RegDev_para* g_RegDev){
	uint32_t value = 0x00000000;
	int	rc = regdev_read(g_RegDev->mem_dev_phy, 0x850, &value);
	if(rc < 0){
		zlog_info(g_RegDev->log_handler,"get_ctrl_frame_crc_correct_cnt failed !!! \n");
		return rc;
	}
	return value;
}

// ctrl_frame_crc_error_cnt -- 0x854
uint32_t get_ctrl_frame_crc_error_cnt(g_RegDev_para* g_RegDev){
	uint32_t value = 0x00000000;
	int	rc = regdev_read(g_RegDev->mem_dev_phy, 0x854, &value);
	if(rc < 0){
		zlog_info(g_RegDev->log_handler,"get_ctrl_frame_crc_error_cnt failed !!! \n");
		return rc;
	}
	return value;
}

// manage_frame_crc_correct_cnt -- 0x858
uint32_t get_manage_frame_crc_correct_cnt(g_RegDev_para* g_RegDev){
	uint32_t value = 0x00000000;
	int	rc = regdev_read(g_RegDev->mem_dev_phy, 0x858, &value);
	if(rc < 0){
		zlog_info(g_RegDev->log_handler,"get_manage_frame_crc_correct_cnt failed !!! \n");
		return rc;
	}
	return value;
}

// manage_frame_crc_error_cnt -- 0x85c
uint32_t get_manage_frame_crc_error_cnt(g_RegDev_para* g_RegDev){
	uint32_t value = 0x00000000;
	int	rc = regdev_read(g_RegDev->mem_dev_phy, 0x85c, &value);
	if(rc < 0){
		zlog_info(g_RegDev->log_handler,"get_manage_frame_crc_error_cnt failed !!! \n");
		return rc;
	}
	return value;
}

// bb_send_cnt -- 0x13c
uint32_t get_bb_send_cnt(g_RegDev_para* g_RegDev){
	uint32_t value = 0x00000000;
	int	rc = regdev_read(g_RegDev->mem_dev_phy, 0x13c, &value);
	if(rc < 0){
		zlog_info(g_RegDev->log_handler,"get_bb_send_cnt failed !!! \n");
		return rc;
	}
	return value;
}

// rx_vector -- 0x84c
uint32_t get_rx_vector(g_RegDev_para* g_RegDev){
	uint32_t value = 0x00000000;
	int	rc = regdev_read(g_RegDev->mem_dev_phy, 0x84c, &value);
	if(rc < 0){
		zlog_info(g_RegDev->log_handler,"get_rx_vector failed !!! \n");
		return rc;
	}
	return value;
}

// pac_soft_rst -- 0x82c -- ddr
uint32_t get_pac_soft_rst(g_RegDev_para* g_RegDev){
	uint32_t value = 0x00000000;
	int	rc = regdev_read(g_RegDev->mem_dev_phy, 0x82c, &value);
	if(rc < 0){
		zlog_info(g_RegDev->log_handler,"get_pac_soft_rst failed !!! \n");
		return rc;
	}
	return value;
}

// sw_fifo_data_cnt -- 0x81c
uint32_t get_sw_fifo_data_cnt(g_RegDev_para* g_RegDev){
	uint32_t value = 0x00000000;
	int	rc = regdev_read(g_RegDev->mem_dev_phy, 0x81c, &value);
	if(rc < 0){
		zlog_info(g_RegDev->log_handler,"get_sw_fifo_data_cnt failed !!! \n");
		return rc;
	}
	return value;
}

// snr -- 0x140

// delay_RW -- 0x868
uint32_t get_delay_RW(g_RegDev_para* g_RegDev){
	uint32_t value = 0x00000000;
	int	rc = regdev_read(g_RegDev->mem_dev_phy, 0x868, &value);
	if(rc < 0){
		zlog_info(g_RegDev->log_handler,"get_delay_RW failed !!! \n");
		return rc;
	}
	return value;
}

uint32_t get_evm(g_RegDev_para* g_RegDev){
	uint32_t evm = 0x0;
	int	rc = regdev_read(g_RegDev->mem_dev_phy, 0x14c, &evm);
	if(rc < 0){
		zlog_info(g_RegDev->log_handler,"get_evm failed !!! \n");
		return rc;
	}
	return evm;
}

uint32_t get_ber(g_RegDev_para* g_RegDev){
	uint32_t ber = 0x0;
	int	rc = regdev_read(g_RegDev->mem_dev_phy, 0x150, &ber);
	if(rc < 0){
		zlog_info(g_RegDev->log_handler,"get_ber failed !!! \n");
		return rc;
	}
	return ber;
}


































