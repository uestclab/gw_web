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


#endif /* RESPONSE_JSON_H */
