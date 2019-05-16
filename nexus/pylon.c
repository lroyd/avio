#include <stdarg.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <assert.h>
#include <signal.h>
#include <errno.h>
#include <time.h>
#include <sys/wait.h>


#include "pylon.h"


#include "list.h"

static struct dl_list alloc_list = DL_LIST_HEAD_INIT(alloc_list);

#define ALLOC_MAGIC 0xa84ef1b2
#define FREED_MAGIC 0x67fd487a

struct os_alloc_trace {
	unsigned int magic;
	struct dl_list list;
	size_t len;

} __attribute__((aligned(16)));




void os_sleep(os_time_t sec, os_time_t usec)
{
	if (sec)
		sleep(sec);
	if (usec)
		usleep(usec);
}


int os_get_time(struct os_time *t)
{
	int res;
	struct timeval tv;
	res = gettimeofday(&tv, NULL);
	t->sec = tv.tv_sec;
	t->usec = tv.tv_usec;
	return res;
}


int os_get_reltime(struct os_reltime *t)
{
#if defined(CLOCK_BOOTTIME)
	static clockid_t clock_id = CLOCK_BOOTTIME;
#elif defined(CLOCK_MONOTONIC)
	static clockid_t clock_id = CLOCK_MONOTONIC;
#else
	static clockid_t clock_id = CLOCK_REALTIME;
#endif
	struct timespec ts;
	int res;

	while (1) 
	{
		res = clock_gettime(clock_id, &ts);
		if (res == 0) 
		{
			t->sec = ts.tv_sec;
			t->usec = ts.tv_nsec / 1000;
			return 0;
		}
		switch (clock_id) 
		{
#ifdef CLOCK_BOOTTIME
		case CLOCK_BOOTTIME:
			clock_id = CLOCK_MONOTONIC;
			break;
#endif
#ifdef CLOCK_MONOTONIC
		case CLOCK_MONOTONIC:
			clock_id = CLOCK_REALTIME;
			break;
#endif
		case CLOCK_REALTIME:
			return -1;
		}
	}

}


int os_mktime(int year, int month, int day, int hour, int min, int sec, os_time_t *t)
{
	struct tm tm, *tm1;
	time_t t_local, t1, t2;
	os_time_t tz_offset;

	if (year < 1970 || month < 1 || month > 12 || day < 1 || day > 31 ||
	    hour < 0 || hour > 23 || min < 0 || min > 59 || sec < 0 ||
	    sec > 60)
		return -1;

	memset(&tm, 0, sizeof(tm));
	tm.tm_year = year - 1900;
	tm.tm_mon = month - 1;
	tm.tm_mday = day;
	tm.tm_hour = hour;
	tm.tm_min = min;
	tm.tm_sec = sec;

	t_local = mktime(&tm);

	/* figure out offset to UTC */
	tm1 = localtime(&t_local);
	if (tm1) 
	{
		t1 = mktime(tm1);
		tm1 = gmtime(&t_local);
		if (tm1) 
		{
			t2 = mktime(tm1);
			tz_offset = t2 - t1;
		} else
			tz_offset = 0;
	} else
		tz_offset = 0;

	*t = (os_time_t) t_local - tz_offset;
	return 0;
}


int os_gmtime(os_time_t t, struct os_tm *tm)
{
	struct tm *tm2;
	time_t t2 = t;

	tm2 = gmtime(&t2);
	if (tm2 == NULL)
		return -1;
	tm->sec = tm2->tm_sec;
	tm->min = tm2->tm_min;
	tm->hour = tm2->tm_hour;
	tm->day = tm2->tm_mday;
	tm->month = tm2->tm_mon + 1;
	tm->year = tm2->tm_year + 1900;
	return 0;
}


#define os_daemon daemon


int os_daemonize(const char *pid_file)
{
	if (os_daemon(0, 0)) 
	{
		perror("daemon");
		return -1;
	}

	if (pid_file) 
	{
		FILE *f = fopen(pid_file, "w");
		if (f) 
		{
			fprintf(f, "%u\n", getpid());
			fclose(f);
		}
	}

	return -0;
}


void os_daemonize_terminate(const char *pid_file)
{
	if (pid_file)
		unlink(pid_file);
}


int os_get_random(unsigned char *buf, size_t len)
{
	FILE *f;
	size_t rc;

	f = fopen("/dev/urandom", "rb");
	if (f == NULL) {
		printf("Could not open /dev/urandom.\n");
		return -1;
	}

	rc = fread(buf, 1, len, f);
	fclose(f);

	return rc != len ? -1 : 0;
}


unsigned long os_random(void)
{
	return random();
}


char *os_rel2abs_path(const char *rel_path)
{
	char *buf = NULL, *cwd, *ret;
	size_t len = 128, cwd_len, rel_len, ret_len;
	int last_errno;

	if (!rel_path)
		return NULL;

	if (rel_path[0] == '/')
		return os_strdup(rel_path);

	for (;;) 
	{
		buf = os_malloc(len);
		if (buf == NULL)
			return NULL;
		cwd = getcwd(buf, len);
		if (cwd == NULL) 
		{
			last_errno = errno;
			os_free(buf);
			if (last_errno != ERANGE)
				return NULL;
			len *= 2;
			if (len > 2000)
				return NULL;
		} else 
		{
			buf[len - 1] = '\0';
			break;
		}
	}

	cwd_len = os_strlen(cwd);
	rel_len = os_strlen(rel_path);
	ret_len = cwd_len + 1 + rel_len + 1;
	ret = os_malloc(ret_len);
	if (ret) 
	{
		os_memcpy(ret, cwd, cwd_len);
		ret[cwd_len] = '/';
		os_memcpy(ret + cwd_len + 1, rel_path, rel_len);
		ret[ret_len - 1] = '\0';
	}
	os_free(buf);
	return ret;
}


int os_setenv(const char *name, const char *value, int overwrite)
{
	return setenv(name, value, overwrite);
}


int os_unsetenv(const char *name)
{
	return unsetenv(name);
}


char *os_readfile(const char *name, size_t *len)
{
	FILE *f;
	char *buf;
	long pos;

	f = fopen(name, "rb");
	if (f == NULL)
		return NULL;

	if (fseek(f, 0, SEEK_END) < 0 || (pos = ftell(f)) < 0) 
	{
		fclose(f);
		return NULL;
	}
	
	*len = pos;
	if (fseek(f, 0, SEEK_SET) < 0) 
	{
		fclose(f);
		return NULL;
	}

	buf = os_malloc(*len);
	if (buf == NULL) 
	{
		fclose(f);
		return NULL;
	}

	if (fread(buf, 1, *len, f) != *len)
	{
		fclose(f);
		os_free(buf);
		return NULL;
	}

	fclose(f);

	return buf;
}


int os_file_exists(const char *fname)
{
	return access(fname, F_OK) == 0;
}

//同步磁盘设备
int os_fdatasync(FILE *stream)
{
	if (!fflush(stream)) 
	{
		return fsync(fileno(stream));
	}

	return -1;
}

static inline int testing_fail_alloc(void)
{
	return 0;
}


void * os_malloc(size_t size)
{
	struct os_alloc_trace *a;

	if (testing_fail_alloc()) return NULL;

	a = malloc(sizeof(*a) + size);
	if (a == NULL)
		return NULL;
	a->magic = ALLOC_MAGIC;
	dl_list_add(&alloc_list, &a->list);
	a->len = size;

	return a + 1;
}

//重新分配, 调整大小
void *os_realloc(void *ptr, size_t size)
{
	struct os_alloc_trace *a;
	size_t copy_len;
	void *n;

	if (ptr == NULL)
		return os_malloc(size);

	a = (struct os_alloc_trace *) ptr - 1;
	if (a->magic != ALLOC_MAGIC) 
	{
		printf("REALLOC[%p]: invalid magic 0x%x%s",
			   a, a->magic,
			   a->magic == FREED_MAGIC ? " (already freed)" : "");
		printf("Invalid os_realloc() call");
		abort();
	}
	n = os_malloc(size);
	if (n == NULL)
		return NULL;
	copy_len = a->len;
	if (copy_len > size)
		copy_len = size;
	os_memcpy(n, a + 1, copy_len);
	os_free(ptr);
	return n;
}


void os_free(void *ptr)
{
	struct os_alloc_trace *a;

	if (ptr == NULL)
		return;
	a = (struct os_alloc_trace *) ptr - 1;
	if (a->magic != ALLOC_MAGIC) 
	{
		printf("FREE[%p]: invalid magic 0x%x%s",
			   a, a->magic,
			   a->magic == FREED_MAGIC ? " (already freed)" : "");
		printf("Invalid os_free() call");
		abort();
	}
	dl_list_del(&a->list);
	a->magic = FREED_MAGIC;

	free(a);
}

//打印当前malloc分配情况
void os_mem_check(void)
{
	
	
}


void * os_memcpy(void *dest, const void *src, size_t n)
{
	char *d = dest;
	const char *s = src;
	while (n--)
		*d++ = *s++;
	return dest;
}


void * os_memmove(void *dest, const void *src, size_t n)
{
	if (dest < src)
		os_memcpy(dest, src, n);
	else 
	{
		/* overlapping areas */
		char *d = (char *) dest + n;
		const char *s = (const char *) src + n;
		while (n--)
			*--d = *--s;
	}
	return dest;
}


void * os_memset(void *s, int c, size_t n)
{
	char *p = s;
	while (n--)
		*p++ = c;
	return s;
}


int os_memcmp(const void *s1, const void *s2, size_t n)
{
	const unsigned char *p1 = s1, *p2 = s2;

	if (n == 0)
		return 0;

	while (*p1 == *p2) 
	{
		p1++;
		p2++;
		n--;
		if (n == 0)
			return 0;
	}

	return *p1 - *p2;
}

void *os_zalloc(size_t size)
{
	void *ptr = os_malloc(size);
	if (ptr)
		os_memset(ptr, 0, size);
	return ptr;
}


char * os_strdup(const char *s)
{
	size_t len;
	char *d;
	len = os_strlen(s);
	d = os_malloc(len + 1);
	if (d == NULL)
		return NULL;
	os_memcpy(d, s, len);
	d[len] = '\0';
	return d;
}


size_t os_strlen(const char *s)
{
	const char *p = s;
	while (*p)
		p++;
	return p - s;
}



int os_strcasecmp(const char *s1, const char *s2)
{
	return os_strcmp(s1, s2);
}


int os_strncasecmp(const char *s1, const char *s2, size_t n)
{
	return os_strncmp(s1, s2, n);
}


char * os_strchr(const char *s, int c)
{
	while (*s) 
	{
		if (*s == c)
			return (char *) s;
		s++;
	}
	return NULL;
}


char * os_strrchr(const char *s, int c)
{
	const char *p = s;
	while (*p)
		p++;
	p--;
	while (p >= s) 
	{
		if (*p == c)
			return (char *) p;
		p--;
	}
	return NULL;
}


int os_strcmp(const char *s1, const char *s2)
{
	while (*s1 == *s2) 
	{
		if (*s1 == '\0')
			break;
		s1++;
		s2++;
	}

	return *s1 - *s2;
}


int os_strncmp(const char *s1, const char *s2, size_t n)
{
	if (n == 0)
		return 0;

	while (*s1 == *s2) 
	{
		if (*s1 == '\0')
			break;
		s1++;
		s2++;
		n--;
		if (n == 0)
			return 0;
	}

	return *s1 - *s2;
}


char * os_strncpy(char *dest, const char *src, size_t n)
{
	char *d = dest;

	while (n--) 
	{
		*d = *src;
		if (*src == '\0')
			break;
		d++;
		src++;
	}

	return dest;
}


size_t os_strlcpy(char *dest, const char *src, size_t siz)
{
	const char *s = src;
	size_t left = siz;

	if (left) 
	{
		/* Copy string up to the maximum size of the dest buffer */
		while (--left != 0) 
		{
			if ((*dest++ = *s++) == '\0')
				break;
		}
	}

	if (left == 0) 
	{
		/* Not enough room for the string; force NUL-termination */
		if (siz != 0)
			*dest = '\0';
		while (*s++)
			; /* determine total src string length */
	}

	return s - src - 1;
}


int os_memcmp_const(const void *a, const void *b, size_t len)
{
	const unsigned char *aa = a;
	const unsigned char *bb = b;
	size_t i;
	unsigned char res;

	for (res = 0, i = 0; i < len; i++)
		res |= aa[i] ^ bb[i];

	return res;
}


char * os_strstr(const char *haystack, const char *needle)
{
	size_t len = os_strlen(needle);
	while (*haystack) 
	{
		if (os_strncmp(haystack, needle, len) == 0)
			return (char *) haystack;
		haystack++;
	}

	return NULL;
}


int os_snprintf(char *str, size_t size, const char *format, ...)
{
	va_list ap;
	int ret;
	
	va_start(ap, format);
	ret = vsnprintf(str, size, format, ap);
	va_end(ap);
	if (size > 0)
		str[size - 1] = '\0';
	return ret;
}

int os_exec(const char *program, const char *arg, int wait_completion)
{
	pid_t pid;
	int pid_status;

	pid = fork();
	if (pid < 0) 
	{
		perror("fork");
		return -1;
	}

	if (pid == 0) 
	{
		/* run the external command in the child process */
		const int MAX_ARG = 30;
		char *_program, *_arg, *pos;
		char *argv[MAX_ARG + 1];
		int i;

		_program = os_strdup(program);
		_arg = os_strdup(arg);

		argv[0] = _program;

		i = 1;
		pos = _arg;
		while (i < MAX_ARG && pos && *pos) 
		{
			while (*pos == ' ')
				pos++;
			if (*pos == '\0')
				break;
			argv[i++] = pos;
			pos = os_strchr(pos, ' ');
			if (pos)
				*pos++ = '\0';
		}
		argv[i] = NULL;

		execv(program, argv);
		perror("execv");
		os_free(_program);
		os_free(_arg);
		exit(0);
		return -1;
	}

	if (wait_completion) 
	{
		/* wait for the child process to complete in the parent */
		waitpid(pid, &pid_status, 0);
	}

	return 0;
}








