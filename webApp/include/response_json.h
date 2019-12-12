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


char* system_state_response(int is_ready, char* fpga_version, char* soft_version, int is_exception);

char* reg_state_response(reg_state_t* reg_state);

char* rssi_data_response(double rssi_data);

char* csi_data_response(float *db_array, float *time_IQ, int len);

char* constell_data_response(int *vectReal, int *vectImag, int len);

/* test */
char* test_json(int op_cmd);

#endif /* RESPONSE_JSON_H */
