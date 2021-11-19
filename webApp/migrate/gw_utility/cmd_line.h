#ifndef _CMD_LINE_H
#define _CMD_LINE_H

#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <linux/ioctl.h>
#include <sys/stat.h>
#include <linux/types.h>
#include <sys/time.h>
#include <dirent.h>
#include <limits.h>
#include <errno.h>
#include <assert.h>
#include <poll.h>

#define	LONG_COMMON_OPT\
	{ "help",	 0, NULL, 'h' },\
	{ "version", 0, NULL, 'v' },\
	{ "file",    1, NULL, 'f' },\
	{ "print",   1, NULL, 'p' },\
	{ "spec",    0, NULL, 's' },\
	{ "debug",   1, NULL, 'd' },\
	{ 0, 0, 0, 0 }

#define	SHORT_COMMON_OPT	"hvf:l:p:s::d:"

typedef struct g_args_para{
	char*   prog_name;
	char*   conf_file;
	char*   log_file;
	int     debug_switch;
}g_args_para;

int parse_cmd_line(int argc, char **argv, g_args_para *g_args);

#endif // _CMD_LINE_H
