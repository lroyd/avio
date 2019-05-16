#ifndef __PYLON_C_H__ 
#define __PYLON_C_H__


#define PYLON_C_LIB_DEFINES		//使用


typedef long os_time_t;


void os_sleep(os_time_t sec, os_time_t usec);

struct os_time {
	os_time_t sec;
	os_time_t usec;
};

struct os_reltime {
	os_time_t sec;
	os_time_t usec;
};

/**
 * os_get_time - Get current time (sec, usec)
 * Returns: 0 on success, -1 on failure
 */
int os_get_time(struct os_time *t);

/**
 * os_get_reltime - Get relative time (sec, usec)
 * Returns: 0 on success, -1 on failure
 */
int os_get_reltime(struct os_reltime *t);


/* a的时间 < b的时间 */
static inline int os_time_before(struct os_time *a, struct os_time *b)
{
	return (a->sec < b->sec) ||
	       (a->sec == b->sec && a->usec < b->usec);
}

/* 获得a - b的时间 res */
static inline void os_time_sub(struct os_time *a, struct os_time *b,
			       struct os_time *res)
{
	res->sec = a->sec - b->sec;
	res->usec = a->usec - b->usec;
	if (res->usec < 0) 
	{
		res->sec--;
		res->usec += 1000000;
	}
}


/* Helpers for handling struct os_reltime */

static inline int os_reltime_before(struct os_reltime *a,
				    struct os_reltime *b)
{
	return (a->sec < b->sec) ||
	       (a->sec == b->sec && a->usec < b->usec);
}


static inline void os_reltime_sub(struct os_reltime *a, struct os_reltime *b,
				  struct os_reltime *res)
{
	res->sec = a->sec - b->sec;
	res->usec = a->usec - b->usec;
	if (res->usec < 0) 
	{
		res->sec--;
		res->usec += 1000000;
	}
}


static inline void os_reltime_age(struct os_reltime *start,
				  struct os_reltime *age)
{
	struct os_reltime now;

	os_get_reltime(&now);
	os_reltime_sub(&now, start, age);
}


static inline int os_reltime_expired(struct os_reltime *now,
				     struct os_reltime *ts,
				     os_time_t timeout_secs)
{
	struct os_reltime age;

	os_reltime_sub(now, ts, &age);
	return (age.sec > timeout_secs) ||
	       (age.sec == timeout_secs && age.usec > 0);
}


static inline int os_reltime_initialized(struct os_reltime *t)
{
	return t->sec != 0 || t->usec != 0;
}


/**
 * os_mktime - Convert broken-down time into seconds since 1970-01-01
 * @year: Four digit year
 * @month: Month (1 .. 12)
 * @day: Day of month (1 .. 31)
 * @hour: Hour (0 .. 23)
 * @min: Minute (0 .. 59)
 * @sec: Second (0 .. 60)
 * @t: Buffer for returning calendar time representation (seconds since
 * 1970-01-01 00:00:00)
 * Returns: 0 on success, -1 on failure
 *
 * Note: The result is in seconds from Epoch, i.e., in UTC, not in local time
 * which is used by POSIX mktime().
 */
int os_mktime(int year, int month, int day, int hour, int min, int sec,
	      os_time_t *t);

struct os_tm {
	int sec; /* 0..59 or 60 for leap seconds */
	int min; /* 0..59 */
	int hour; /* 0..23 */
	int day; /* 1..31 */
	int month; /* 1..12 */
	int year; /* Four digit year */
};

int os_gmtime(os_time_t t, struct os_tm *tm);

/**
 * os_daemonize - Run in the background (detach from the controlling terminal)
 * @pid_file: File name to write the process ID to or %NULL to skip this
 * Returns: 0 on success, -1 on failure
 */
int os_daemonize(const char *pid_file);

/**
 * os_daemonize_terminate - Stop running in the background (remove pid file)
 * @pid_file: File name to write the process ID to or %NULL to skip this
 */
void os_daemonize_terminate(const char *pid_file);

int os_get_random(unsigned char *buf, size_t len);

unsigned long os_random(void);

/**
 * os_rel2abs_path - Get an absolute path for a file
 */
char * os_rel2abs_path(const char *rel_path);


/**
 * os_setenv - Set environment variable
 * Returns: 0 on success, -1 on error
 */
int os_setenv(const char *name, const char *value, int overwrite);
int os_unsetenv(const char *name);

/**
读文件，需要os_free释放
 */
char *os_readfile(const char *name, size_t *len);

/** 检查文件是否存在，1存在 0不存在
 */
int os_file_exists(const char *fname);


int os_fdatasync(FILE *stream);

/* os_zalloc - Allocate and zero memory */
void *os_zalloc(size_t size);

/**
 * os_calloc - Allocate and zero memory for an array
 * @nmemb: Number of members in the array
 * @size: Number of bytes in each member
 */
static inline void *os_calloc(size_t nmemb, size_t size)
{
	if (size && nmemb > (~(size_t) 0) / size)
		return NULL;
	return os_zalloc(nmemb * size);
}

#ifdef PYLON_C_LIB_DEFINES

void *os_malloc(size_t size);
//ptr == NULL <=> os_malloc
void *os_realloc(void *ptr, size_t size);
void os_free(void *ptr);

void os_mem_check(void);

void *os_memcpy(void *dest, const void *src, size_t n);
void *os_memmove(void *dest, const void *src, size_t n);
void *os_memset(void *s, int c, size_t n);
int os_memcmp(const void *s1, const void *s2, size_t n);
char *os_strdup(const char *s);
size_t os_strlen(const char *s);
int os_strcasecmp(const char *s1, const char *s2);
int os_strncasecmp(const char *s1, const char *s2, size_t n);
char *os_strchr(const char *s, int c);
char *os_strrchr(const char *s, int c);
int os_strcmp(const char *s1, const char *s2);
int os_strncmp(const char *s1, const char *s2, size_t n);
char * os_strstr(const char *haystack, const char *needle);
int os_snprintf(char *str, size_t size, const char *format, ...);

#else
	
#ifndef os_malloc
#define os_malloc(s) malloc((s))
#endif
#ifndef os_realloc
#define os_realloc(p, s) realloc((p), (s))
#endif
#ifndef os_free
#define os_free(p) free((p))
#endif
#ifndef os_strdup
#define os_strdup(s) strdup(s)
#endif
#ifndef os_memcpy
#define os_memcpy(d, s, n) memcpy((d), (s), (n))
#endif
#ifndef os_memmove
#define os_memmove(d, s, n) memmove((d), (s), (n))
#endif
#ifndef os_memset
#define os_memset(s, c, n) memset(s, c, n)
#endif
#ifndef os_memcmp
#define os_memcmp(s1, s2, n) memcmp((s1), (s2), (n))
#endif
#ifndef os_strlen
#define os_strlen(s) strlen(s)
#endif
#ifndef os_strcasecmp
#define os_strcasecmp(s1, s2) strcasecmp((s1), (s2))
#endif
#ifndef os_strncasecmp
#define os_strncasecmp(s1, s2, n) strncasecmp((s1), (s2), (n))
#endif
#ifndef os_strchr
#define os_strchr(s, c) strchr((s), (c))
#endif
#ifndef os_strcmp
#define os_strcmp(s1, s2) strcmp((s1), (s2))
#endif
#ifndef os_strncmp
#define os_strncmp(s1, s2, n) strncmp((s1), (s2), (n))
#endif
#ifndef os_strrchr
#define os_strrchr(s, c) strrchr((s), (c))
#endif
#ifndef os_strstr
#define os_strstr(h, n) strstr((h), (n))
#endif
#ifndef os_snprintf
#define os_snprintf snprintf
#endif

#endif



static inline int os_snprintf_error(size_t size, int res)
{
	return res < 0 || (unsigned int) res >= size;
}

static inline void * os_realloc_array(void *ptr, size_t nmemb, size_t size)
{
	if (size && nmemb > (~(size_t)0) / size)
		return NULL;
	return os_realloc(ptr, nmemb * size);
}

/**
 * os_remove_in_array - Remove a member from an array by index
 * @ptr: Pointer to the array
 * @nmemb: Current member count of the array
 * @size: The size per member of the array
 * @idx: Index of the member to be removed
 */
static inline void os_remove_in_array(void *ptr, size_t nmemb, size_t size,
				      size_t idx)
{
	if (idx < nmemb - 1)
		os_memmove(((unsigned char *) ptr) + idx * size,
			   ((unsigned char *) ptr) + (idx + 1) * size,
			   (nmemb - idx - 1) * size);
}

size_t os_strlcpy(char *dest, const char *src, size_t siz);

int os_memcmp_const(const void *a, const void *b, size_t len);

/**
 * @wait_completion: Whether to wait until the program execution completes
 */
int os_exec(const char *program, const char *arg, int wait_completion);



#endif 
