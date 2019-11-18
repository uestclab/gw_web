#include "small_utility.h"
#include <string.h>
#include <math.h>
#include "cJSON.h"


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