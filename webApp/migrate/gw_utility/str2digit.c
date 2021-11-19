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


static unsigned long long ret_ERANGE(void)
{
	errno = ERANGE; /* this ain't as small as it looks (on glibc) */
	return 0;//ULLONG_MAX;
}

static int bb_ascii_isalnum(unsigned char a)
{
	unsigned char b = a - '0';
	if (b <= 9)
		return (b <= 9);
	b = (a|0x20) - 'a';
	return b <= 'z' - 'a';
}

static unsigned long long handle_errors(unsigned long long v, char **endp)
{
	char next_ch = **endp;

	/* errno is already set to ERANGE by strtoXXX if value overflowed */
	if (next_ch) {
		/* "1234abcg" or out-of-range? */
		if (bb_ascii_isalnum(next_ch) || errno)
			return ret_ERANGE();
		/* good number, just suspicious terminator */
		errno = EINVAL;
	}
	return v;
}

int bb_strtoull(const char *arg, char **endp, int base)
{
	unsigned long long v;
	char *endptr;
	int index = 0;

	if (!endp) endp = &endptr;
	*endp = (char*) arg;

	/* strtoul("  -4200000000") returns 94967296, errno 0 (!) */
	/* I don't think that this is right. Preventing this... */
	if (!bb_ascii_isalnum(arg[0])){
		if('-' == arg[0]){
			index = 1;
		}else{
			return ret_ERANGE();
		}
	}

	/* not 100% correct for lib func, but convenient for the caller */
	errno = 0;
	v = strtoull(&arg[index], endp, base);
	v = handle_errors(v, endp);
	return index?-v:v;
}


int get_item_val(char *str)
{
	int ret = -1;
	char hex_str_lc[] = "0x";
	char hex_str_uc[] = "0X";
	int base = 10;


	if(strstr(str,hex_str_lc) || \
		strstr(str,hex_str_uc)){
		base = 16;
	}
			
	ret = bb_strtoull(str, NULL, base);
	return ret;
}

