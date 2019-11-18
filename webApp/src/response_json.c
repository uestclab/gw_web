#include <assert.h>
#include "response_json.h"



char* system_state_response(int is_ready, char* fpga_version, char* soft_version, int is_exception){
    char* system_state_response_json = NULL;
    if(is_ready == 1){
        cJSON *root = cJSON_CreateObject();
        cJSON_AddStringToObject(root, "comment", "system is ready");
        if(is_exception == 0){
            cJSON_AddNumberToObject(root, "type", TYPE_SYSTEM_STATE_RESPONSE);
        }else{
            cJSON_AddNumberToObject(root, "type", TYPE_SYSTEM_STATE_EXCEPTION);
        }
        cJSON_AddNumberToObject(root, "array_number", 3);

        cJSON *array=cJSON_CreateArray();

        cJSON *obj_state=cJSON_CreateObject();
        cJSON_AddStringToObject(obj_state, "name","system_state");
        cJSON_AddStringToObject(obj_state, "data_type","int");
        cJSON_AddNumberToObject(obj_state, "value",1);
        cJSON_AddItemToArray(array,obj_state);

        cJSON *obj_fpga=cJSON_CreateObject();
        cJSON_AddStringToObject(obj_fpga, "name","fpga_version");
        cJSON_AddStringToObject(obj_fpga, "data_type","string");
        assert(fpga_version != NULL);
        cJSON_AddStringToObject(obj_fpga, "value",fpga_version);
        cJSON_AddItemToArray(array,obj_fpga);

        cJSON *obj_soft=cJSON_CreateObject();
        cJSON_AddStringToObject(obj_soft, "name","soft_version");
        cJSON_AddStringToObject(obj_soft, "data_type","string");
        assert(soft_version != NULL);
        cJSON_AddStringToObject(obj_soft, "value",soft_version);
        cJSON_AddItemToArray(array,obj_soft);

        cJSON_AddItemToObject(root,"ret_value",array);
        system_state_response_json = cJSON_Print(root);
        cJSON_Delete(root);
    }else{
        cJSON *root = cJSON_CreateObject();
        cJSON_AddStringToObject(root, "comment", fpga_version);
        if(is_exception == 0){
            cJSON_AddNumberToObject(root, "type", TYPE_SYSTEM_STATE_RESPONSE);
        }else{
            cJSON_AddNumberToObject(root, "type", TYPE_SYSTEM_STATE_EXCEPTION);
        }
        cJSON_AddNumberToObject(root, "array_number", 1);

        cJSON *array=cJSON_CreateArray();

        cJSON *obj_state=cJSON_CreateObject();
        cJSON_AddStringToObject(obj_state, "name","system_state");
        cJSON_AddStringToObject(obj_state, "data_type","int");
        cJSON_AddNumberToObject(obj_state, "value",0);
        cJSON_AddItemToArray(array,obj_state);

        cJSON_AddItemToObject(root,"ret_value",array);
        system_state_response_json = cJSON_Print(root);
        cJSON_Delete(root);
    }
    
    return system_state_response_json;
}

char* reg_state_response(reg_state_t* reg_state){
    char* reg_state_response_json = NULL;
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "comment", "reg_state_response");
    cJSON_AddNumberToObject(root, "type", TYPE_REG_STATE_RESPONSE);
    cJSON_AddNumberToObject(root, "array_number", reg_state->reg_state_num);

    cJSON *array=cJSON_CreateArray();

    cJSON *obj_1=cJSON_CreateObject();
    cJSON_AddStringToObject(obj_1, "name","power_est_latch");
    cJSON_AddStringToObject(obj_1, "data_type","unsigned int");
    cJSON_AddNumberToObject(obj_1, "value",reg_state->power_est_latch);
    cJSON_AddItemToArray(array,obj_1);

    cJSON *obj_2=cJSON_CreateObject();
    cJSON_AddStringToObject(obj_2, "name","coarse_low16");
    cJSON_AddStringToObject(obj_2, "data_type","unsigned int");
    cJSON_AddNumberToObject(obj_2, "value",reg_state->coarse_low16);
    cJSON_AddItemToArray(array,obj_2);

    cJSON *obj_3=cJSON_CreateObject();
    cJSON_AddStringToObject(obj_3, "name","fine_high16");
    cJSON_AddStringToObject(obj_3, "data_type","unsigned int");
    cJSON_AddNumberToObject(obj_3, "value",reg_state->fine_high16);
    cJSON_AddItemToArray(array,obj_3);

    cJSON *obj_4=cJSON_CreateObject();
    cJSON_AddStringToObject(obj_4, "name","freq_offset");
    cJSON_AddStringToObject(obj_4, "data_type","double");
    cJSON_AddNumberToObject(obj_4, "value",reg_state->freq_offset);
    cJSON_AddItemToArray(array,obj_4);

    cJSON *obj_5=cJSON_CreateObject();
    cJSON_AddStringToObject(obj_5, "name","tx_modulation");
    cJSON_AddStringToObject(obj_5, "data_type","int");
    cJSON_AddNumberToObject(obj_5, "value",reg_state->tx_modulation);
    cJSON_AddItemToArray(array,obj_5);

    cJSON *obj_6=cJSON_CreateObject();
    cJSON_AddStringToObject(obj_6, "name","dac_state");
    cJSON_AddStringToObject(obj_6, "data_type","int");
    cJSON_AddNumberToObject(obj_6, "value",reg_state->dac_state);
    cJSON_AddItemToArray(array,obj_6);

    cJSON *obj_7=cJSON_CreateObject();
    cJSON_AddStringToObject(obj_7, "name","snr");
    cJSON_AddStringToObject(obj_7, "data_type","double");
    cJSON_AddNumberToObject(obj_7, "value",reg_state->snr);
    cJSON_AddItemToArray(array,obj_7);

    cJSON *obj_8=cJSON_CreateObject();
    cJSON_AddStringToObject(obj_8, "name","distance");
    cJSON_AddStringToObject(obj_8, "data_type","double");
    cJSON_AddNumberToObject(obj_8, "value",reg_state->distance);
    cJSON_AddItemToArray(array,obj_8);

    cJSON_AddItemToObject(root,"ret_value",array);
    reg_state_response_json = cJSON_Print(root);
    cJSON_Delete(root);
    return reg_state_response_json;
}

char* rssi_data_response(double rssi_data){
    char* rssi_data_response_json = NULL;
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "comment", "rssi_data_response");
    cJSON_AddNumberToObject(root, "type", TYPE_RSSI_DATA_RESPONSE);
    cJSON_AddNumberToObject(root, "array_number", 1);

    cJSON *array=cJSON_CreateArray();

    cJSON *obj_1=cJSON_CreateObject();
    cJSON_AddStringToObject(obj_1, "name","rssi");
    cJSON_AddStringToObject(obj_1, "data_type","double");
    cJSON_AddNumberToObject(obj_1, "value",rssi_data);
    cJSON_AddItemToArray(array,obj_1);    

    cJSON_AddItemToObject(root,"ret_value",array);
    rssi_data_response_json = cJSON_Print(root);
    cJSON_Delete(root);
    return rssi_data_response_json;
}