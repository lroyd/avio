#ifndef __NEXUS_CORE_H__
#define __NEXUS_CORE_H__

#include <stddef.h>

#include "pylon.h"


/**
 * ELOOP_ALL_CTX - eloop_cancel_timeout() magic number to match all timeouts
 */
#define ELOOP_ALL_CTX (void *) -1


typedef enum {
	EVENT_TYPE_READ = 0,
	EVENT_TYPE_WRITE,
	EVENT_TYPE_EXCEPTION
} eloop_event_type;


typedef void (*eloop_sock_handler)(int sock, void *eloop_ctx, void *sock_ctx);

typedef void (*eloop_event_handler)(void *eloop_data, void *user_ctx);

typedef void (*eloop_timeout_handler)(void *eloop_data, void *user_ctx);

typedef void (*eloop_signal_handler)(int sig, void *signal_ctx);


int eloop_init(void);

int eloop_register_read_sock(int sock, eloop_sock_handler handler, void *eloop_data, void *user_data);
void eloop_unregister_read_sock(int sock);

int eloop_register_sock(int sock, eloop_event_type type, eloop_sock_handler handler, void *eloop_data, void *user_data);
void eloop_unregister_sock(int sock, eloop_event_type type);

/* linux 下不可用  */
int eloop_register_event(void *event, size_t event_size, eloop_event_handler handler, void *eloop_data, void *user_data);
void eloop_unregister_event(void *event, size_t event_size);

/* 定时器超时后会自动删除，实现周期触发的话，在超时函数中自行再次注册 */
int eloop_register_timeout(unsigned int secs, unsigned int usecs, eloop_timeout_handler handler, void *eloop_data, void *user_data);

			   
int eloop_cancel_timeout(eloop_timeout_handler handler, void *eloop_data, void *user_data);

/** 删除对应的定时器，返回剩余时间 */
int eloop_cancel_timeout_one(eloop_timeout_handler handler,
			     void *eloop_data, void *user_data,
			     struct os_reltime *remaining);

/**检查定时器是否被注册Returns: 1 if the timeout is registered, 0 if the timeout is not registered */
int eloop_is_timeout_registered(eloop_timeout_handler handler, void *eloop_data, void *user_data);

/**
 * Find a registered matching <handler,eloop_data,user_data> timeout. If found,
 * deplete the timeout if remaining time is more than the requested time.
 */
int eloop_deplete_timeout(unsigned int req_secs, unsigned int req_usecs, eloop_timeout_handler handler, void *eloop_data, void *user_data);
int eloop_replenish_timeout(unsigned int req_secs, unsigned int req_usecs, eloop_timeout_handler handler, void *eloop_data, void *user_data);

/* 自行注册 */
int eloop_register_signal(int sig, eloop_signal_handler handler, void *user_data);
/* 注册SIGINT, SIGTERM */
int eloop_register_signal_terminate(eloop_signal_handler handler, void *user_data);

/**
 * This function is a more portable version of eloop_register_signal() since
 * the knowledge of exact details of the signals is hidden in eloop
 * implementation. In case of operating systems using signal(), this function
 * registers a handler for SIGHUP.
 */
int eloop_register_signal_reconfig(eloop_signal_handler handler, void *user_data);

int eloop_sock_requeue(void);

//没有超时，和关闭所有监听的fd后会退出
void eloop_run(void);

void eloop_terminate(void);

//释放所有资源
void eloop_destroy(void);

/**
 * 检查eloop_terminate是否已经被调用: 1 = event loop terminate, 0 = event loop still running
 */
int eloop_terminated(void);

/** Do a blocking wait for a single read socket. */
void eloop_wait_for_read_sock(int sock);

#endif
