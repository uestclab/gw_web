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

#include "fftw3.h"
#include <math.h>

#define PI 3.1415926
#define NUM_FREQ_OFFSET  6839.503//2*pi*55*2*10-9*8

void postMsg(long int msg_type, char *buf, int buf_len, void* data, g_msg_queue_para* g_msg_queue);
char* c_compiler_builtin_macro();

unsigned int stringToInt(char* ret);
char* parse_fpga_version(uint32_t number);

double calculateFreq(uint32_t number);

/* fft ---- csi  */
void parse_IQ_from_net(char* buf, int len, fftwf_complex *in_IQ); // length must be 1024
void calculate_spectrum(fftwf_complex *in_IQ , fftwf_complex *out_fft, fftwf_plan *p, float* spectrum, int len); // len = 256
int myfftshift(float* db_array, float* spectrum, int len); // len = 256
void timeDomainChange(fftwf_complex *in_IQ, float* time_IQ, int len); // len = 256

#ifdef __cplusplus
    }
#endif

#endif  /* __SMALL_UTILITY_H__ */