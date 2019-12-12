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
        cJSON_AddStringToObject(root, "comment", "system is not ready");
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
/* --------------------------------------------------------------------------- */
    cJSON *obj_9=cJSON_CreateObject();
    cJSON_AddStringToObject(obj_9, "name","txc_retrans_cnt");
    cJSON_AddStringToObject(obj_9, "data_type","unsigned int");
    cJSON_AddNumberToObject(obj_9, "value",reg_state->txc_retrans_cnt);
    cJSON_AddItemToArray(array,obj_9);

    cJSON *obj_10=cJSON_CreateObject();
    cJSON_AddStringToObject(obj_10, "name","expect_seq_id_low16");
    cJSON_AddStringToObject(obj_10, "data_type","unsigned int");
    cJSON_AddNumberToObject(obj_10, "value",reg_state->expect_seq_id_low16);
    cJSON_AddItemToArray(array,obj_10);

    cJSON *obj_11=cJSON_CreateObject();
    cJSON_AddStringToObject(obj_11, "name","expect_seq_id_high16");
    cJSON_AddStringToObject(obj_11, "data_type","unsigned int");
    cJSON_AddNumberToObject(obj_11, "value",reg_state->expect_seq_id_high16);
    cJSON_AddItemToArray(array,obj_11);

    cJSON *obj_12=cJSON_CreateObject();
    cJSON_AddStringToObject(obj_12, "name","rx_id");
    cJSON_AddStringToObject(obj_12, "data_type","unsigned int");
    cJSON_AddNumberToObject(obj_12, "value",reg_state->rx_id);
    cJSON_AddItemToArray(array,obj_12);

    cJSON *obj_13=cJSON_CreateObject();
    cJSON_AddStringToObject(obj_13, "name","rx_fifo_data");
    cJSON_AddStringToObject(obj_13, "data_type","unsigned int");
    cJSON_AddNumberToObject(obj_13, "value",reg_state->rx_fifo_data);
    cJSON_AddItemToArray(array,obj_13);

    cJSON *obj_14=cJSON_CreateObject();
    cJSON_AddStringToObject(obj_14, "name","rx_sync");
    cJSON_AddStringToObject(obj_14, "data_type","unsigned int");
    cJSON_AddNumberToObject(obj_14, "value",reg_state->rx_sync);
    cJSON_AddItemToArray(array,obj_14);

    cJSON *obj_15=cJSON_CreateObject();
    cJSON_AddStringToObject(obj_15, "name","rx_v_agg_num");
    cJSON_AddStringToObject(obj_15, "data_type","unsigned int");
    cJSON_AddNumberToObject(obj_15, "value",reg_state->rx_v_agg_num);
    cJSON_AddItemToArray(array,obj_15);

    cJSON *obj_16=cJSON_CreateObject();
    cJSON_AddStringToObject(obj_16, "name","rx_v_len");
    cJSON_AddStringToObject(obj_16, "data_type","unsigned int");
    cJSON_AddNumberToObject(obj_16, "value",reg_state->rx_v_len);
    cJSON_AddItemToArray(array,obj_16);

    cJSON *obj_17=cJSON_CreateObject();
    cJSON_AddStringToObject(obj_17, "name","tx_abort");
    cJSON_AddStringToObject(obj_17, "data_type","unsigned int");
    cJSON_AddNumberToObject(obj_17, "value",reg_state->tx_abort);
    cJSON_AddItemToArray(array,obj_17);

    cJSON *obj_18=cJSON_CreateObject();
    cJSON_AddStringToObject(obj_18, "name","ddr_closed");
    cJSON_AddStringToObject(obj_18, "data_type","unsigned int");
    cJSON_AddNumberToObject(obj_18, "value",reg_state->ddr_closed);
    cJSON_AddItemToArray(array,obj_18);

    cJSON *obj_19=cJSON_CreateObject();
    cJSON_AddStringToObject(obj_19, "name","sw_fifo_cnt");
    cJSON_AddStringToObject(obj_19, "data_type","unsigned int");
    cJSON_AddNumberToObject(obj_19, "value",reg_state->sw_fifo_cnt);
    cJSON_AddItemToArray(array,obj_19);

    cJSON *obj_20=cJSON_CreateObject();
    cJSON_AddStringToObject(obj_20, "name","bb_send_cnt");
    cJSON_AddStringToObject(obj_20, "data_type","unsigned int");
    cJSON_AddNumberToObject(obj_20, "value",reg_state->bb_send_cnt);
    cJSON_AddItemToArray(array,obj_20);

    cJSON *obj_21=cJSON_CreateObject();
    cJSON_AddStringToObject(obj_21, "name","ctrl_crc_c");
    cJSON_AddStringToObject(obj_21, "data_type","unsigned int");
    cJSON_AddNumberToObject(obj_21, "value",reg_state->ctrl_crc_c);
    cJSON_AddItemToArray(array,obj_21);

    cJSON *obj_22=cJSON_CreateObject();
    cJSON_AddStringToObject(obj_22, "name","ctrl_crc_e");
    cJSON_AddStringToObject(obj_22, "data_type","unsigned int");
    cJSON_AddNumberToObject(obj_22, "value",reg_state->ctrl_crc_e);
    cJSON_AddItemToArray(array,obj_22);

    cJSON *obj_23=cJSON_CreateObject();
    cJSON_AddStringToObject(obj_23, "name","manage_crc_c");
    cJSON_AddStringToObject(obj_23, "data_type","unsigned int");
    cJSON_AddNumberToObject(obj_23, "value",reg_state->manage_crc_c);
    cJSON_AddItemToArray(array,obj_23);

    cJSON *obj_24=cJSON_CreateObject();
    cJSON_AddStringToObject(obj_24, "name","manage_crc_e");
    cJSON_AddStringToObject(obj_24, "data_type","unsigned int");
    cJSON_AddNumberToObject(obj_24, "value",reg_state->manage_crc_e);
    cJSON_AddItemToArray(array,obj_24);


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

/* spectrum array(db_array) and time_IQ */
char* csi_data_response(float *db_array, float *time_IQ, int len){

    int db_int[256];
    int time_int[256];

    for(int i=0;i<256;i++){
        db_int[i] = (int)db_array[i];
        time_int[i] = (int)(time_IQ[i] * 10000);
    }

    char* csi_data_response_json = NULL;
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "comment", "csi_data_response");
    cJSON_AddNumberToObject(root, "type", TYPE_CSI_DATA_RESPONSE);
    cJSON_AddNumberToObject(root, "array_number", len);

    cJSON_AddItemToObject(root, "ret_spectrum_array", cJSON_CreateIntArray(db_int, len));
    cJSON_AddItemToObject(root, "ret_time_array", cJSON_CreateIntArray(time_int, len));


    csi_data_response_json = cJSON_Print(root);
    cJSON_Delete(root);
    return csi_data_response_json;
}

char* constell_data_response(int *vectReal, int *vectImag, int len){

    char* constell_data_response_json = NULL;
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "comment", "constell_data_response");
    cJSON_AddNumberToObject(root, "type", TYPE_CONSTELLATION_DATA_RESPONSE);
    cJSON_AddNumberToObject(root, "array_number", len);

    cJSON *array=cJSON_CreateArray();

    int iq_pair[2];
    for(int i=0;i<len;i++){
        iq_pair[0] = vectReal[i];
        iq_pair[1] = vectImag[i];
        cJSON_AddItemToArray(array,cJSON_CreateIntArray(iq_pair, 2));
    }

    cJSON_AddItemToObject(root,"ret_iq_array",array);

    constell_data_response_json = cJSON_Print(root);
    cJSON_Delete(root);
    return constell_data_response_json;
}

/* test */
char* test_json(int op_cmd){
    char* test_json = NULL;
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "comment", "test_json");
    cJSON_AddNumberToObject(root, "type", TYPE_CONTROL_SAVE_CSI);
    cJSON_AddStringToObject(root, "dst","csi");
    cJSON_AddNumberToObject(root, "op_cmd",op_cmd);

	cJSON_AddStringToObject(root, "file_name","csi");

    test_json = cJSON_Print(root);
    cJSON_Delete(root);
    return test_json;
}


