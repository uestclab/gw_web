#include "small_utility.h"
#include <string.h>
#include <math.h>
#include "cJSON.h"

void postMsg(long int msg_type, char *buf, int buf_len, void* tmp_data, g_msg_queue_para* g_msg_queue){
	struct msg_st data;
	data.msg_type = msg_type;
	data.msg_number = msg_type;

	data.tmp_data = tmp_data;

	data.msg_len = buf_len;
	if(buf != NULL && buf_len != 0)
		memcpy(data.msg_json,buf,buf_len);

	postMsgQueue(&data,g_msg_queue);
}

unsigned int stringToInt(char* ret){
	if(ret == NULL){
		return -1;
	}
	int i;
	int len = strlen(ret);
	if(len < 3)
		return -1;
	if(ret[0] != '0' || ret[1] != 'x')
		return -1;
	unsigned int number = 0;
	int flag = 0;
	for(i=2;i<len;i++){
		if(ret[i] == '0' && flag == 0)
			continue;
		flag = 1;
		int temp = 0;
		if(ret[i]>='0'&&ret[i]<='9')
			temp = ret[i] - '0';
		else if(ret[i]>='a'&&ret[i]<='f')
			temp = ret[i] - 'a' + 10;
		else if(ret[i]>='A'&&ret[i]<='F')
			temp = ret[i] - 'A' + 10;
		if(i == len - 1)
			number = number + temp;
		else
			number = (number + temp)*16;
	}
	return number;
}


/* fpga_version */
char* intTimeToString(int number){
	char* temp = malloc(32);
	
	if(number < 10){// 1
		temp[0] = '0';
		temp[1] = '0' + (char)number;
		temp[2] = '\0';
	}else if(number >= 2000){ // 4
		char a[4];
		a[0] = 2;
		a[1] = 0;
		a[2] = (char)((number - 2000)/10);
		a[3] = (char)((number - 2000)%10);
		temp[0] = a[0] + '0';
		temp[1] = a[1] + '0';
		temp[2] = a[2] + '0';
		temp[3] = a[3] + '0';
		temp[4] = '\0';
	}else{ // 2
		temp[0] = (char)(number/10) + '0';
		temp[1] = (char)(number%10) + '0';
		temp[2] = '\0';
	}
	return temp;
}

char* parse_fpga_version(uint32_t number){
	char* version = malloc(32);
	int day,month,year,hours,minutes,seconds;
	char* tmp = NULL;
	seconds = number - ((number>>6)<<6);
	tmp = intTimeToString(seconds);
	memcpy(version + 12,tmp,2);
	free(tmp);
	tmp = NULL;
	
	number = number >> 6;
	minutes = number -((number>>6)<<6);
	tmp = intTimeToString(minutes);
	memcpy(version + 10,tmp,2);
	free(tmp);
	tmp = NULL;
	
	number = number >> 6;
	hours = number - ((number>>5)<<5);
	tmp = intTimeToString(hours);
	memcpy(version + 8,tmp,2);
	free(tmp);
	tmp = NULL;
	
	number = number >> 5;
	year = 2000 + number - ((number>>6)<<6);
	tmp = intTimeToString(year);
	memcpy(version,tmp,4);
	free(tmp);
	tmp = NULL;
	
	number = number >> 6;
	month = number - ((number>>4)<<4);
	tmp = intTimeToString(month);
	memcpy(version + 4,tmp,2);
	free(tmp);
	tmp = NULL;
	
	number = number >> 4;
	day = number;
	tmp = intTimeToString(day);
	memcpy(version+6,tmp,2);
	free(tmp);
	tmp = NULL;
	
	version[14] = '\0';
	
	return version;
}

char* c_compiler_builtin_macro()
{
    char* build_version = (char *) malloc(strlen(__DATE__) + strlen(__TIME__) + 16);
    sprintf(build_version, "%s - %s", __DATE__, __TIME__);
    return build_version;
}

double calculateFreq(uint32_t number){ // 0x04FF04FF , 0x04FF0400 , 0x04000400
	int freq_offset_i = 0, freq_offset_q =0, freq_offset_i_pre=0, freq_offset_q_pre=0;

	freq_offset_q_pre = number - ((number>>8) << 8);
	number = number >> 8;
	freq_offset_i_pre = number - ((number>>8) << 8);
	number = number >> 8;
	freq_offset_q = number - ((number>>8) << 8);
	number = number >> 8;
	freq_offset_i = number - ((number>>8) << 8);

	if(freq_offset_i > 127)
		freq_offset_i = freq_offset_i - 256;

	if(freq_offset_q > 127)
		freq_offset_q = freq_offset_q - 256;

	if(freq_offset_i_pre > 127)
		freq_offset_i_pre = freq_offset_i_pre - 256;

	if(freq_offset_q_pre > 127)
		freq_offset_q_pre = freq_offset_q_pre - 256;

	double temp_1 = atan((freq_offset_q * 1.0)/(freq_offset_i * 1.0));
	double temp_2 = atan((freq_offset_q_pre * 1.0)/(freq_offset_i_pre * 1.0));
	double diff = temp_1 - temp_2;
	double result = (diff / 5529.203) * 1000000000;
	return result;
}

/* fft ------------ CSI */
void swap(char*a){
	char temp;
	temp = a[0];
	a[0] = a[1];
	a[1] = temp;
}

float tranform(char *input,int length){
	float value_d = 0;
	unsigned short temp = 0;
	
	char a[2];
	memcpy(a,input,2);
	swap(a);
	temp = *((unsigned short*)a);
	
	int value_i = temp;
	if(value_i > 32767)
		value_i = value_i - 65536;
	value_d =  (value_i / 1.0) / 32768;
	return value_d;
}

// length must be 1024 -- 2 byte each I or Q data
void parse_IQ_from_net(char* buf, int len, fftwf_complex *in_IQ){
	char input_c[2];
	int counter = 0;
	int stream_counter = 0;
	double IQ_stream[256*2];
	int i;
	for(i=0;i<len;i++){
		input_c[counter] = *(buf + i);
		counter = counter + 1;
		if(counter == 2){
			float temp_d = tranform(input_c,2);
			IQ_stream[stream_counter] = temp_d;
			stream_counter = stream_counter + 1;
			counter = 0;
		}
	}
	stream_counter = 0;
	for(i=0;i<512;i = i + 16){
		// real
		in_IQ[stream_counter][0]   = IQ_stream[i];
		in_IQ[stream_counter+1][0] = IQ_stream[i+1];
		in_IQ[stream_counter+2][0] = IQ_stream[i+2];
		in_IQ[stream_counter+3][0] = IQ_stream[i+3];
		in_IQ[stream_counter+4][0] = IQ_stream[i+4];
		in_IQ[stream_counter+5][0] = IQ_stream[i+5];
		in_IQ[stream_counter+6][0] = IQ_stream[i+6];
		in_IQ[stream_counter+7][0] = IQ_stream[i+7];
		// imaginary
		in_IQ[stream_counter][1]   = IQ_stream[i+8];
		in_IQ[stream_counter+1][1] = IQ_stream[i+9];
		in_IQ[stream_counter+2][1] = IQ_stream[i+10];
		in_IQ[stream_counter+3][1] = IQ_stream[i+11];
		in_IQ[stream_counter+4][1] = IQ_stream[i+12];
		in_IQ[stream_counter+5][1] = IQ_stream[i+13];
		in_IQ[stream_counter+6][1] = IQ_stream[i+14];
		in_IQ[stream_counter+7][1] = IQ_stream[i+15];
		stream_counter = stream_counter + 8;
	}
}

// len = 256
void calculate_spectrum(fftwf_complex *in_IQ , fftwf_complex *out_fft, fftwf_plan *p, float* spectrum, int len){
	// fft
	*p = fftwf_plan_dft_1d(len, in_IQ, out_fft, FFTW_FORWARD, FFTW_ESTIMATE);
	fftwf_execute(*p);

	for (int i = 0; i < len; i++){
		spectrum[i] = (out_fft[i][0]*out_fft[i][0] + out_fft[i][1]*out_fft[i][1]) / (256*256);
	}
}

// len = 256
int myfftshift(float* db_array, float* spectrum, int len){
	int i;
	for(i=0;i<len;i++){
		if(spectrum[i] < 1e-100){
			return -1;
		}
		spectrum[i] = 10.0 * log10(spectrum[i]);
	}
	memcpy(db_array,spectrum+128,sizeof(float)*128);
	memcpy(db_array+128,spectrum,sizeof(float)*128);
	return 0;
}

// len = 256
void timeDomainChange(fftwf_complex *in_IQ,float* time_IQ, int len){
	int i;
	for(i=0;i<len;i++){
		time_IQ[i] = sqrt(in_IQ[i][0] * in_IQ[i][0] + in_IQ[i][1] * in_IQ[i][1]);
		time_IQ[i] = exp(time_IQ[i]);
	}
}



















/* --- tmp ---- */

/* i2c interface */
int i2cset(const char* dev, const char* addr, const char* reg, int size, const char* data){
	int     ret = -1;
	char*   jsonfile = NULL;
	char*   stat_buf = NULL;
	int     stat_buf_len = 0;
	cJSON * root = NULL; 
	cJSON * item = NULL;
	cJSON * array = NULL;
	cJSON * arrayobj = NULL;
	
	root = cJSON_CreateObject();
	cJSON_AddStringToObject(root, "dev", dev); // "/dev/i2c-0" , "/dev/i2c-1"

	cJSON_AddStringToObject(root, "addr", addr);
	cJSON_AddStringToObject(root, "force", "0x1");
	cJSON_AddStringToObject(root, "dst", "rf");
	array=cJSON_CreateArray();
	cJSON_AddItemToObject(root,"op_cmd",array);

	arrayobj=cJSON_CreateObject();
	cJSON_AddItemToArray(array,arrayobj);
	cJSON_AddStringToObject(arrayobj, "_comment","R0");
	cJSON_AddStringToObject(arrayobj, "cmd","1"); // i2cset
	cJSON_AddStringToObject(arrayobj, "reg",reg);
	if(size == 0)
		cJSON_AddStringToObject(arrayobj, "size","0");
	else if(size == 1){
		cJSON_AddStringToObject(arrayobj, "size","1");
		cJSON_AddStringToObject(arrayobj, "data",data); // "0x00"
	}

	jsonfile = cJSON_Print(root);
	item = cJSON_GetObjectItem(root,"dst");
	//ret = dev_transfer(jsonfile,strlen(jsonfile), &stat_buf, &stat_buf_len, item->valuestring, -1);

	cJSON_Delete(root);
	root = NULL;
	free(jsonfile);

	if(ret == 0 && stat_buf_len > 0){

		root = cJSON_Parse(stat_buf);
		item = cJSON_GetObjectItem(root,"stat");

		if(strcmp(item->valuestring,"0") != 0){
			;//zlog_info(g_broker->log_handler,"i2cset ----- buf_json = %s\n",stat_buf);
		}

		cJSON_Delete(root);
		root = NULL;
		free(stat_buf);
	}
	
	return 0;
}

char* i2cget(const char* dev, const char* addr, const char* reg, int size){
	int     ret = -1;
	char*   jsonfile = NULL;
	char*   stat_buf = NULL;
	int     stat_buf_len = 0;
	cJSON * root = NULL; 
	cJSON * item = NULL;
	cJSON * array = NULL;
	cJSON * arrayobj = NULL;
	char *  ret_return = NULL;

	root = cJSON_CreateObject();
	cJSON_AddStringToObject(root, "dev", dev);
	cJSON_AddStringToObject(root, "addr", addr);
	cJSON_AddStringToObject(root, "force", "0x1");
	cJSON_AddStringToObject(root, "dst", "rf");
	array=cJSON_CreateArray();
	cJSON_AddItemToObject(root,"op_cmd",array);

	arrayobj=cJSON_CreateObject();
	cJSON_AddItemToArray(array,arrayobj);
	cJSON_AddStringToObject(arrayobj, "_comment","R0");
	cJSON_AddStringToObject(arrayobj, "cmd","0"); // i2cget
	cJSON_AddStringToObject(arrayobj, "reg",reg);
	if(size == 1)
		cJSON_AddStringToObject(arrayobj, "size","1");
	else if(size == 2)
		cJSON_AddStringToObject(arrayobj, "size","2");

	jsonfile = cJSON_Print(root);
	item = cJSON_GetObjectItem(root,"dst");
	//ret = dev_transfer(jsonfile,strlen(jsonfile), &stat_buf, &stat_buf_len, item->valuestring, -1);

	cJSON_Delete(root);
	root = NULL;
	free(jsonfile);

	if(ret == 0 && stat_buf_len > 0){

		cJSON * arry_return = NULL;

		root = cJSON_Parse(stat_buf);
		item = cJSON_GetObjectItem(root,"stat");
		if(strcmp(item->valuestring,"0") != 0){
			;//zlog_info(g_broker->log_handler,"i2cget ----- buf_json = %s\n",stat_buf);
			cJSON_Delete(root);
			root = NULL;
			free(stat_buf);
			return NULL;
		}

		arry_return = cJSON_GetObjectItem(root,"return");
		if(NULL != arry_return){
			cJSON* temp_list = arry_return->child;
			while(temp_list != NULL){
				char * ret_tmp   = cJSON_GetObjectItem( temp_list , "ret")->valuestring;
				temp_list = temp_list->next;
				ret_return = malloc(32);
				memcpy(ret_return,ret_tmp,strlen(ret_tmp)+1);
			}
		}
		cJSON_Delete(root);
		root = NULL;
		free(stat_buf);
	}
	
	return ret_return;
}