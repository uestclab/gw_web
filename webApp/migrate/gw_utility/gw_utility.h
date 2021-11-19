#ifndef GW_UTILITY_H
#define GW_UTILITY_H

#ifdef __cplusplus
extern "C" {
#endif


int32_t myNtohl(const char* buf);
int filelength(FILE *fp);
char* readfile(const char *path);
int64_t now();

void hexdump(const void* p, size_t size);
//void hexdump_zlog(const void* p, size_t size, zlog_category_t *zlog_handler);
int get_mac(char * mac, int len_limit, char *arg);
int get_ip(char* ip, char* arg);

void change_mac_buf(char* in_addr, char* out_addr);

char* getHigh16Str(char* mac);
char* getLow32Str(char* mac);

uint32_t getLow32(char* dst);
uint32_t getHigh16(char* dst);

void reverseBuf(char* in_buf, char* out_buf, int number);

// -- temp 
void user_wait();
void gw_sleep();

unsigned char cal_table_high_first(unsigned char value);

/* create lookup table */
void  create_crc_table(void);

/* crc 8 */
unsigned char crc_high_first(unsigned char *ptr, unsigned char len);

unsigned char cal_crc_table(unsigned char *ptr, unsigned char len);

void print_buf(char* buf, int length);

unsigned char crc_high_first_by_init_crc(unsigned char *ptr, unsigned char len, unsigned char init_crc);

unsigned char cal_crc_table_by_init_crc(unsigned char *ptr, unsigned char len, unsigned char init_crc);

/* -------------------------------- aligned memory allocate ------------------------------------------- */

void* aligned32_malloc(size_t size);

void aligned32_free(void* ptr);

#ifdef __cplusplus
}
#endif

#endif//GW_UTILITY_H
