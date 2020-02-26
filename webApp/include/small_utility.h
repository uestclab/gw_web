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

void postMsg(long int msg_type, char *buf, int buf_len, void* tmp_data, int tmp_data_len, g_msg_queue_para* g_msg_queue);
char* c_compiler_builtin_macro();

unsigned int stringToDecimalInt(char* ret);
unsigned int stringToInt(char* ret);
char* parse_fpga_version(uint32_t number);

double calculateFreq(uint32_t number);

/* fft ---- csi  */
void parse_IQ_from_net(char* buf, int len, fftwf_complex *in_IQ); // length must be 1024
void calculate_spectrum(fftwf_complex *in_IQ , fftwf_complex *out_fft, fftwf_plan *p, float* spectrum, int len); // len = 256
int myfftshift(float* db_array, float* spectrum, int len); // len = 256
void timeDomainChange(fftwf_complex *in_IQ, float* time_IQ, int len); // len = 256

/* ------------------ constellation IQ data process ------------ */
int checkIQ(char input);

int IsProcessIsRun(char *proc);

/* i2c interface */
int i2cset(const char* dev, const char* addr, const char* reg, int size, const char* data);
char* i2cget(const char* dev, const char* addr, const char* reg, int size);

double calculateDeviceTemp(char* ret);
double calculate_rf_cur(char* rfcur_low,char* rfcur_high);
double calculate_local_oscillator_lock(char* rfcur_low,char* rfcur_high);
double calculate_rf_temper(char* rfcur_low,char* rfcur_high);
double calculateBBCurrent(char* ret);
double calculateBBVs(char* ret);
double calculateADCTemper(char* ret);

#ifdef __cplusplus
    }
#endif

#endif  /* __SMALL_UTILITY_H__ */