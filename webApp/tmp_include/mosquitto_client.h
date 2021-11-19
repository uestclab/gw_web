#ifndef GW_MOSQ_CLIENT_H
#define GW_MOSQ_CLIENT_H

typedef int (*sub_cb)(char *buf, int buf_len, char *topic, void *userdata); 
typedef int (*exception_cb)(void  *userdata); 
typedef int (*log_print)(const char *format,...);


struct mosq_user{
	char *prog_name;
	char **sub_topics;
	int topic_count;
	sub_cb sub_callback;
    exception_cb exception_callback;
    log_print log_print_callback;
	void *userdata;
};

int start_mosquitto_client(struct mosq_user *user_config);
void stop_mosquitto_client();

int my_publish(const char *topic, int payloadlen, void *payload);


#endif// GW_MOSQ_CLIENT_H