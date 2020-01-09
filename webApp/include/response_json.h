#ifndef RESPONSE_JSON_H
#define RESPONSE_JSON_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include "cJSON.h"
#include "zlog.h"
#include "gw_utility.h"
#include "web_common.h"

typedef struct system_state_t{
    char* system_str;
    char* fpga_version;
    char* soft_version;
    int   dac_state;
    int   distance_state;
    int   frequency;
    int   tx_power_state;
    int   rx_gain_state;
}system_state_t;


char* system_state_response(int is_ready, int is_exception, system_state_t* tmp_system_state);

char* reg_state_response(reg_state_t* reg_state);

char* rssi_data_response(double rssi_data);

char* csi_data_response(float *db_array, float *time_IQ, int len);

char* constell_data_response(int *vectReal, int *vectImag, int len);

char* cmd_state_response(int state);

/* test */
char* test_json(int op_cmd);

#endif /* RESPONSE_JSON_H */
