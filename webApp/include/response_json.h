#ifndef RESPONSE_JSON_H
#define RESPONSE_JSON_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include "cJSON.h"
#include "zlog.h"
#include "gw_utility.h"
#include "gw_register.h"
#include "web_common.h"

/**@struct system_state_t
* @brief 定义页面每次刷新或打开请求系统信息数据类型
*/
typedef struct system_state_t{
    char  system_str[256];
    char* fpga_version;
    char* soft_version;
    int   dac_state;
    int   distance_state;
    int   frequency;
    int   tx_power_state;
    int   rx_gain_state;
}system_state_t;

typedef struct record_str_t{
    char constrol_rssi_succ[128];
    char constrol_rssi_fail[128];
    char start_csi_succ[128];
    char start_csi_fail[128];
    char start_csi_mutex[128];
    char stop_csi_succ[128];
    char stop_csi_fail[128];
    char start_constell_succ[128];
    char start_constell_fail[128];
    char start_constell_mutex[128];
    char stop_constell_succ[128];
    char stop_constell_fail[128];
    char csi_save_succ[128];
    char csi_save_fail[128];
    char open_distance_succ[128];
    char close_distance_succ[128];
    char open_dac_succ[128];
    char close_dac_succ[128];
    char clear_log_succ[128];
    char reset_sys_succ[128];
    char ip_setting_succ[128];
    char open_txpower_succ[128];
    char close_txpower_succ[128];
    char open_rxgain_succ[128];
    char close_rxgain_succ[128];
}record_str_t;

void init_record_str(record_str_t* record);

char* system_state_response(int is_ready, int is_exception, system_state_t* tmp_system_state);

char* reg_state_response(reg_state_t* reg_state);

char* rssi_data_response(double rssi_data, int seq_num);

char* csi_data_response(float *db_array, float *time_IQ, int len);

char* constell_data_response(int *vectReal, int *vectImag, int len);

char* cmd_state_response(int state, char* record_str);

char* statistics_response(g_RegDev_para* g_RegDev,int64_t start,int64_t update_acc_time,zlog_category_t* handler);

char* rf_info_response(g_RegDev_para* g_RegDev,zlog_category_t* handler);

/* test */
char* test_json(int op_cmd);

#endif /* RESPONSE_JSON_H */
