#ifndef __SMALL_UTILITY_H__
#define __SMALL_UTILITY_H__

#ifdef __cplusplus
    extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include "msg_queue.h"

#define PI 3.1415
#define NUM_FREQ_OFFSET  6839.503//2*pi*55*2*10-9*8

void postMsg(long int msg_type, char *buf, int buf_len, g_msg_queue_para* g_msg_queue);
char* c_compiler_builtin_macro();

unsigned int stringToInt(char* ret);
char* parse_fpga_version(uint32_t number);

double calculateFreq(uint32_t number);

#ifdef __cplusplus
    }
#endif

#endif  /* __SMALL_UTILITY_H__ */
