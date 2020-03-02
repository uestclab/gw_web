/* 
    1. process RF info and setting .... dependence mosquitto_broker
*/
#include "rf_module.h"
#include "response_json.h"
#include "small_utility.h"

int frequency_rf;
int tx_power_state;
int rx_gain_state;

/* new 20200225 */
double local_oscillator_lock_state;
double rf_temper;
double rf_current;

double bb_current;
double device_temper;
double bb_vs;
double adc_temper;
double zynq_temper;

/**@defgroup RF rf_process_module.
* @{
* @ingroup rf module
* @brief 提供射频信息数据. \n
* 响应设置射频等操作
*/
char* inquiry_rf_info(g_broker_para* g_broker){
	char* response_json = rf_info_response(g_broker->g_RegDev,g_broker->log_handler);
    return response_json;
}

int process_rf_freq_setting(char* stat_buf, int stat_buf_len, g_broker_para* g_broker){
    zlog_info(g_broker->log_handler, "rf frequency setting :%s \n", stat_buf);
	cJSON * root = NULL;
    cJSON * item = NULL;
    root = cJSON_Parse(stat_buf);
    item = cJSON_GetObjectItem(root,"frequency");
    frequency_rf = stringToDecimalInt(item->valuestring);
    zlog_info(g_broker->log_handler,"item->valuestring = %s , %d", item->valuestring, frequency_rf);
	cJSON_Delete(root);
	return 0;
}

int open_tx_power(){
    tx_power_state = 1;
    return 0;
}

int close_tx_power(){
    tx_power_state = 0;
    return 0;
}

int rx_gain_normal(){
    rx_gain_state = 0;
    return 0;
}

int rx_gain_high(){
    rx_gain_state = 1;
    return 0;
}

// i2cset -y -f 1 0x48 0x19 0x12
// #等待1ms
// i2cget -y -f 1 0x48 0x08
// i2cget -y -f 1 0x48 0x04
double get_local_oscillator_lock_state(zlog_category_t* handler){
    char* p_ret_high = NULL;
    char* p_ret_low = NULL;
    //i2cset  -y -f 0 0x48 0x19 0x12
    i2cset("/dev/i2c-1", "0x48", "0x19", 1, "0x12");

    // i2cget -y -f 0 0x48 0x08     //MSB    高8位
    p_ret_high = i2cget("/dev/i2c-1", "0x48", "0x08", 0);

    // i2cget -y -f 0 0x48 0x04     //Bit[1:0]作为低2位
    p_ret_low  = i2cget("/dev/i2c-1", "0x48", "0x04", 0);

    // // i2cset  -y -f 0 0x48 0x19 0x14  
    // i2cset("/dev/i2c-1", "0x48", "0x19", 1, "0x14");

    double Vadc1 = -1;
    if(p_ret_low != NULL && p_ret_high != NULL){
        Vadc1 = calculate_local_oscillator_lock(p_ret_low, p_ret_high);
        free(p_ret_low);
        free(p_ret_high);
    }
    return Vadc1;
}

// ---射频读取发射电路-1温度 ： 毫米波模块温度？
// i2cset -y -f 1 0x48 0x19 0x15
// #等待1ms
// i2cget -y -f 1 0x48 0x0b
// i2cget -y -f 1 0x48 0x04
// ---射频读取发射电路-2温度 : 毫米波模块温度?
// i2cset -y -f 1 0x48 0x19 0x13
// #等待1ms
// i2cget -y -f 1 0x48 0x09
// i2cget -y -f 1 0x48 0x04
double get_rf_temper(){
    char* p_ret_high = NULL;
    char* p_ret_low = NULL;
    //i2cset  -y -f 0 0x48 0x19 0x15
    i2cset("/dev/i2c-1", "0x48", "0x19", 1, "0x15");
    usleep(1000);
    //i2cget -y -f 0 0x48 0x0b    //MSB    高8位
    p_ret_high = i2cget("/dev/i2c-1", "0x48", "0x0b", 0);

    // i2cget -y -f 0 0x48 0x04     //Bit[7:6]作为低2位
    p_ret_low  = i2cget("/dev/i2c-1", "0x48", "0x04", 0);

    // // i2cset  -y -f 0 0x48 0x19 0x14  
    // i2cset("/dev/i2c-1", "0x48", "0x19", 1, "0x14");

    double Vadc4 = -1;
    if(p_ret_low != NULL && p_ret_high != NULL){
        Vadc4 = calculate_rf_temper(p_ret_low, p_ret_high);
        free(p_ret_low);
        free(p_ret_high);
    }
    return Vadc4;
}

// ---射频读取总电流 : 毫米波模块总电流
// i2cset -y -f 1 0x48 0x19 0x14
// #等待1ms
// i2cget -y -f 1 0x48 0x0a
// i2cget -y -f 1 0x48 0x04
double get_rf_current(zlog_category_t* handler){
    char* p_ret_high = NULL;
    char* p_ret_low = NULL;
	
    //i2cset  -y -f 0 0x48 0x19 0x14
	i2cset("/dev/i2c-1", "0x48", "0x19", 1, "0x14");
    usleep(1000);
	//i2cget -y -f 0 0x48 0x09    :  MSB--高8位
	p_ret_high = i2cget("/dev/i2c-1", "0x48", "0x0a", 0);

	//i2cget -y -f 0 0x48 0x04    :  Bit[3:2]作为低2位
	p_ret_low  = i2cget("/dev/i2c-1", "0x48", "0x04", 0);

    // //i2cset  -y -f 0 0x48 0x19 0x14
    // i2cset("/dev/i2c-1", "0x48", "0x19", 1, "0x14");

    double rf_current = -1;
    if(p_ret_low != NULL && p_ret_high != NULL){
    //     rf_current = calculate_rf_cur(p_ret_low, p_ret_high);
    //     zlog_info(handler,"rf_current = %d\n", rf_current);
        free(p_ret_low);
        free(p_ret_high);
    }
    return rf_current;
}

// ---读取12V电源电流 : 基带板电流
// i2cset -y -f 1 0x28 0x04
// i2cset -y -f 1 0x28 0x01 0x20
//    #等待1ms
// i2cget -y -f 1 0x28 0x04  w    #连续输出两个字节
// i2cset -y -f 1 0x28 0x00       #切换为温度读取通道
// i2cset -y -f 1 0x28 0x01 0x00
double get_bb_current(){
    //  i2cset -y -f 1 0x28 0x04            
    //  i2cset -y -f 1 0x28 0x01 0x20     
    //  i2cget -y -f 1 0x28 0x04  w    //此处会得到两个字节的结果，取高10bit；注意该结果高8位先出，低8位后出，详见附件“IIC参数换算表”
    //  i2cset -y -f 1 0x28 0x00         
    //  i2cset -y -f 1 0x28 0x01 0x00
    char* p_ret = NULL;
	i2cset("/dev/i2c-1", "0x28", "0x04", 0, NULL);
	i2cset("/dev/i2c-1", "0x28", "0x01", 1, "0x20");
	p_ret = i2cget("/dev/i2c-1", "0x28", "0x04", 2);
    i2cset("/dev/i2c-1", "0x28", "0x00", 0, NULL);
	i2cset("/dev/i2c-1", "0x28", "0x01", 1, "0x00");
    double bb_current = -1;

    if(p_ret != NULL){
        bb_current = calculateBBCurrent(p_ret);
        free(p_ret);
    }

    return bb_current;
}

// i2cset -y -f 1 0x28 0x00
// i2cset -y -f 1 0x28 0x01 0x00
//    #等待1ms
// i2cget -y -f 1 0x28 0x00  w   #连续输出两个字节
double get_device_temper(zlog_category_t* handler){
    char* p_ret = NULL;
	
    //i2cset -y -f 1 0x28 0x00
	i2cset("/dev/i2c-1", "0x28", "0x00", 0, NULL);

	//i2cset -y -f 1 0x28 0x01 0x00
	i2cset("/dev/i2c-1", "0x28", "0x01", 1, "0x00");

	//i2cget -y -f 1 0x28 0x00
	p_ret = i2cget("/dev/i2c-1", "0x28", "0x00", 2);
    double device_temper = -1;
    
    if(p_ret != NULL){
        device_temper = calculateDeviceTemp(p_ret);
        //zlog_info(handler," ---------------- device_temper = %f ",device_temper);
        free(p_ret);
    }

	return device_temper;
}

// ---读取12V电源电压 : 基带板电压VS
// i2cset -y -f 1 0x28 0x04
// i2cset -y -f 1 0x28 0x01 0x60
//    #等待1ms
// i2cget -y -f 1 0x28 0x04  w    #连续输出两个字节
// i2cset -y -f 1 0x28 0x00       #切换为温度读取通道
// i2cset -y -f 1 0x28 0x01 0x00
double get_bb_vs(){
    // i2cset -y -f 1 0x28 0x04             
    // i2cset -y -f 1 0x28 0x01 0x60   
    // i2cget -y -f 1 0x28 0x04 w     //此处会得到两个字节的结果，取高10bit；注意该结果高8位先出，低8位后出，详见附件“IIC参数换算表”
    // i2cset -y -f 1 0x28 0x00     
    // i2cset -y -f 1 0x28 0x01 0x00

    char* p_ret = NULL;
	i2cset("/dev/i2c-1", "0x28", "0x04", 0, NULL);
	i2cset("/dev/i2c-1", "0x28", "0x01", 1, "0x60");
	p_ret = i2cget("/dev/i2c-1", "0x28", "0x04", 2);
    i2cset("/dev/i2c-1", "0x28", "0x00", 0, NULL);
	i2cset("/dev/i2c-1", "0x28", "0x01", 1, "0x00");

    double bb_vs = -1;
    
    if(p_ret != NULL){
    // // Vadc3=(ADC Code/1024)*2.5V
    // // VS=Vadc3/0.091
        bb_vs = calculateBBVs(p_ret);
        free(p_ret);
    }

    return bb_vs;
}

double get_adc_temper(){
    // i2cset -y -f 1 0x28 0x04                  
    // i2cset -y -f 1 0x28 0x01 0x80    
    // i2cget -y -f 1 0x28 0x04 w     //此处会得到两个字节的结果，取高10bit；注意该结果高8位先出，低8位后出，详见附件“IIC参数换算表”
    // i2cset -y -f 1 0x28 0x00          
    // i2cset -y -f 1 0x28 0x01 0x00

    char* p_ret = NULL;
	// i2cset("/dev/i2c-1", "0x28", "0x04", 0, NULL);
	// i2cset("/dev/i2c-1", "0x28", "0x01", 1, "0x80");
	// p_ret = i2cget("/dev/i2c-1", "0x28", "0x04", 2);
    // i2cset("/dev/i2c-1", "0x28", "0x00", 0, NULL);
	// i2cset("/dev/i2c-1", "0x28", "0x01", 1, "0x00");

    double adc_temper = -1;
    
    if(p_ret != NULL){
    // // Vadc4=(ADC Code/1024)*2.5V
    // // TJADC=(0.689-Vadc4)/0.0019
        adc_temper = calculateADCTemper(p_ret);
        free(p_ret);
    }
    return adc_temper;
}

double get_zynq_temper(zlog_category_t* handler){

    FILE *pf;
    char buffer[4096];
  
    pf = popen("xadc", "r");
    fread(buffer, sizeof(buffer), 1, pf);
    buffer[32] = '\0';
    //zlog_info(handler, "get_zynq_temper : %s\n", buffer);
      
    pclose(pf);

    double zynq_temper = -1;
    int count = 0;
    int start_idx = -1;
    int end_idx = -1;
    int dot_idx = -1;
    for(int i=0;i<4096;i++){
        if(buffer[i] == ' '){
            count++;
            if(count == 3){
                end_idx = i;
                break;
            }else if(count == 2){
                start_idx = i;
            }
        }
        if(buffer[i] == '.'){
            dot_idx = i;
        }
    }

    //zlog_info(handler, "start_idx=%d,  dot_idx=%d, end_idx=%d\n", start_idx,dot_idx,end_idx);
    char tmp[32];
    memcpy(tmp,&buffer[start_idx+1], dot_idx-start_idx-1);
    memcpy(tmp+dot_idx-start_idx-1,&buffer[dot_idx+1], end_idx-dot_idx-1);
    tmp[end_idx-start_idx] = '\0';

    unsigned int integer = stringToDecimalInt(tmp);
    //zlog_info(handler, "tmp : %s,  integer--%u\n", tmp,integer);

    double fraction_d = integer * 1.0;

    for(int i=0;i<end_idx-dot_idx-1;i++){
        fraction_d = fraction_d / 10.0;
    }

    zynq_temper = ((int)(fraction_d * 10000)) / 10000.0;

    return zynq_temper;
}

/** @} RF*/


