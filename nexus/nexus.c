#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <assert.h>

 

#include "list.h"
#include "nexus.h"


#include <sys/epoll.h>



typedef struct eloop_sock 
{
	int sock;
	void *eloop_data;
	void *user_data;
	eloop_sock_handler handler;
}T_EloopSocket;

struct eloop_timeout 
{
	struct dl_list list;
	struct os_reltime time;
	void *eloop_data;
	void *user_data;
	eloop_timeout_handler handler;
};

struct eloop_signal 
{
	int sig;
	void *user_data;
	eloop_signal_handler handler;
	int signaled;
};

struct eloop_sock_table 
{
	int count;	
	struct eloop_sock *table;
	eloop_event_type type;
	int changed;
};

struct eloop_data 
{
	int max_sock;

	int count;

	int max_fd;
	struct eloop_sock *fd_table;

	int epollfd;
	int epoll_max_event_num;
	struct epoll_event *epoll_events;


	struct eloop_sock_table readers;
	struct eloop_sock_table writers;
	struct eloop_sock_table exceptions;

	struct dl_list timeout;

	int signal_count;
	struct eloop_signal *signals;
	int signaled;
	int pending_terminate;

	int terminate;
};

static struct eloop_data eloop;


int eloop_init(void)
{
	os_memset(&eloop, 0, sizeof(eloop));
	dl_list_init(&eloop.timeout);

	eloop.epollfd = epoll_create1(0);
	if (eloop.epollfd < 0) 
	{
		printf("%s: epoll_create1 failed. %s",  __func__, strerror(errno));
		return -1;
	}

	eloop.readers.type = EVENT_TYPE_READ;
	eloop.writers.type = EVENT_TYPE_WRITE;
	eloop.exceptions.type = EVENT_TYPE_EXCEPTION;

	return 0;
}


//将sock加入epoll监听
static int eloop_sock_queue(int sock, eloop_event_type type)
{
	struct epoll_event ev;

	os_memset(&ev, 0, sizeof(ev));
	switch (type) 
	{
		case EVENT_TYPE_READ:
			ev.events = EPOLLIN;
			break;
		case EVENT_TYPE_WRITE:
			ev.events = EPOLLOUT;
			break;
		/* 可能注册了一个socket*only*用于异常处理 */
		case EVENT_TYPE_EXCEPTION:
			ev.events = EPOLLERR | EPOLLHUP;
			break;
	}
	
	ev.data.fd = sock;
	if (epoll_ctl(eloop.epollfd, EPOLL_CTL_ADD, sock, &ev) < 0) 
	{
		printf("%s: epoll_ctl(ADD) for fd=%d failed: %s", __func__, sock, strerror(errno));
		return -1;
	}
	return 0;
}


//table:全局eloop中读写类的链表头
//1.加入table->table, table->count++;
//2.eloop.count++;
//3.memcpy(&eloop.fd_table[sock], &table->table[table->count - 1])  为什么是fd_table[sock]作为角标？
static int eloop_sock_table_add_sock(struct eloop_sock_table *table,
                                     int sock, eloop_sock_handler handler,
                                     void *eloop_data, void *user_data)
{
	struct epoll_event *temp_events;
	struct eloop_sock *temp_table;
	int next;

	struct eloop_sock *tmp;
	int new_max_sock;

	if (sock > eloop.max_sock)
		new_max_sock = sock;
	else
		new_max_sock = eloop.max_sock;

	if (table == NULL)
		return -1;



	if (new_max_sock >= eloop.max_fd) 
	{
		next = eloop.max_fd == 0 ? 16 : eloop.max_fd * 2;
		temp_table = os_realloc_array(eloop.fd_table, next, sizeof(struct eloop_sock));
		if (temp_table == NULL)
			return -1;

		eloop.max_fd = next;
		eloop.fd_table = temp_table;
	}



	if (eloop.count + 1 > eloop.epoll_max_event_num) 
	{
		next = eloop.epoll_max_event_num == 0 ? 8 : eloop.epoll_max_event_num * 2;
		temp_events = os_realloc_array(eloop.epoll_events, next, sizeof(struct epoll_event));
		if (temp_events == NULL) 
		{
			printf("%s: malloc for epoll failed: %s", __func__, strerror(errno));
			return -1;
		}

		eloop.epoll_max_event_num = next;
		eloop.epoll_events = temp_events;
	}

	tmp = os_realloc_array(table->table, table->count + 1, sizeof(struct eloop_sock));
	if (tmp == NULL) 
	{
		return -1;
	}

	tmp[table->count].sock = sock;
	tmp[table->count].eloop_data = eloop_data;
	tmp[table->count].user_data = user_data;
	tmp[table->count].handler = handler;

	table->count++;
	table->table = tmp;
	eloop.max_sock = new_max_sock;
	eloop.count++;
	table->changed = 1;	//??

	//将fd加入epoll监听
	if (eloop_sock_queue(sock, table->type) < 0) return -1;
	
	os_memcpy(&eloop.fd_table[sock], &table->table[table->count - 1], sizeof(struct eloop_sock));

	return 0;
}


static void eloop_sock_table_remove_sock(struct eloop_sock_table *table, int sock)
{

	int i;

	if (table == NULL || table->table == NULL || table->count == 0)
		return;

	for (i = 0; i < table->count; i++) 
	{
		if (table->table[i].sock == sock)
			break;
	}
	
	if (i == table->count) return;

	if (i != table->count - 1) 
	{
		os_memmove(&table->table[i], &table->table[i + 1], (table->count - i - 1) * sizeof(struct eloop_sock));
	}
	
	table->count--;
	eloop.count--;
	table->changed = 1;

	if (epoll_ctl(eloop.epollfd, EPOLL_CTL_DEL, sock, NULL) < 0) 
	{
		printf("%s: epoll_ctl(DEL) for fd=%d failed: %s", __func__, sock, strerror(errno));
		return;
	}
	os_memset(&eloop.fd_table[sock], 0, sizeof(struct eloop_sock));
}


static void eloop_sock_table_dispatch(struct epoll_event *events, int nfds)
{
	struct eloop_sock *table;
	int i;

	for (i = 0; i < nfds; i++) 
	{
		table = &eloop.fd_table[events[i].data.fd];
		if (table->handler == NULL)
			continue;
		table->handler(table->sock, table->eloop_data, table->user_data);
		if (eloop.readers.changed || eloop.writers.changed || eloop.exceptions.changed)
			break;
	}
}



int eloop_sock_requeue(void)
{
	int r = 0;

	return r;
}


static void eloop_sock_table_destroy(struct eloop_sock_table *table)
{
	if (table) 
	{
		int i;
		for (i = 0; i < table->count && table->table; i++) 
		{
			printf("ELOOP: remaining socket: "
				   "sock=%d eloop_data=%p user_data=%p "
				   "handler=%p",
				   table->table[i].sock,
				   table->table[i].eloop_data,
				   table->table[i].user_data,
				   table->table[i].handler);
		}
		os_free(table->table);
	}
}


int eloop_register_read_sock(int sock, eloop_sock_handler handler, void *eloop_data, void *user_data)
{
	return eloop_register_sock(sock, EVENT_TYPE_READ, handler, eloop_data, user_data);
}


void eloop_unregister_read_sock(int sock)
{
	eloop_unregister_sock(sock, EVENT_TYPE_READ);
}

//根据注册的类型返回表头
static struct eloop_sock_table *eloop_get_sock_table(eloop_event_type type)
{
	switch (type) 
	{
		case EVENT_TYPE_READ:
			return &eloop.readers;
		case EVENT_TYPE_WRITE:
			return &eloop.writers;
		case EVENT_TYPE_EXCEPTION:
			return &eloop.exceptions;
	}

	return NULL;
}


int eloop_register_sock(int sock, eloop_event_type type,
			eloop_sock_handler handler,
			void *eloop_data, void *user_data)
{
	struct eloop_sock_table *table;
	table = eloop_get_sock_table(type);
	return eloop_sock_table_add_sock(table, sock, handler, eloop_data, user_data);
}


void eloop_unregister_sock(int sock, eloop_event_type type)
{
	struct eloop_sock_table *table;
	table = eloop_get_sock_table(type);
	eloop_sock_table_remove_sock(table, sock);
}

//注册超时函数
int eloop_register_timeout(unsigned int secs, unsigned int usecs,
			   eloop_timeout_handler handler,
			   void *eloop_data, void *user_data)
{
	struct eloop_timeout *timeout, *tmp;
	os_time_t now_sec;

	timeout = os_zalloc(sizeof(*timeout));
	if (timeout == NULL) return -1;
	
	if (os_get_reltime(&timeout->time) < 0) //得到当前时间
	{
		os_free(timeout);
		return -1;
	}
	
	now_sec = timeout->time.sec;
	timeout->time.sec += secs;
	if (timeout->time.sec < now_sec) 
	{
		printf("ELOOP: Too long timeout (secs=%u) to " "ever happen - ignore it", secs);
		os_free(timeout);
		return 0;
	}
	
	timeout->time.usec += usecs;
	while (timeout->time.usec >= 1000000) 
	{
		timeout->time.sec++;
		timeout->time.usec -= 1000000;
	}
	
	timeout->eloop_data = eloop_data;
	timeout->user_data = user_data;
	timeout->handler = handler;

	dl_list_for_each(tmp, &eloop.timeout, struct eloop_timeout, list) 
	{
		if (os_reltime_before(&timeout->time, &tmp->time)) 
		{
			dl_list_add(tmp->list.prev, &timeout->list);
			return 0;
		}
	}
	dl_list_add_tail(&eloop.timeout, &timeout->list);

	return 0;
}


static void eloop_remove_timeout(struct eloop_timeout *timeout)
{
	dl_list_del(&timeout->list);
	os_free(timeout);
}


int eloop_cancel_timeout(eloop_timeout_handler handler,
			 void *eloop_data, void *user_data)
{
	struct eloop_timeout *timeout, *prev;
	int removed = 0;

	dl_list_for_each_safe(timeout, prev, &eloop.timeout, struct eloop_timeout, list) 
	{
		if (timeout->handler == handler && (timeout->eloop_data == eloop_data || eloop_data == ELOOP_ALL_CTX) &&
		    (timeout->user_data == user_data || user_data == ELOOP_ALL_CTX)) 
		{
			eloop_remove_timeout(timeout);
			removed++;
		}
	}

	return removed;
}


int eloop_cancel_timeout_one(eloop_timeout_handler handler,
			     void *eloop_data, void *user_data,
			     struct os_reltime *remaining)
{
	struct eloop_timeout *timeout, *prev;
	int removed = 0;
	struct os_reltime now;

	os_get_reltime(&now);
	remaining->sec = remaining->usec = 0;

	dl_list_for_each_safe(timeout, prev, &eloop.timeout, struct eloop_timeout, list) 
	{
		if (timeout->handler == handler && (timeout->eloop_data == eloop_data) && (timeout->user_data == user_data)) 
		{
			removed = 1;
			if (os_reltime_before(&now, &timeout->time)) 
				os_reltime_sub(&timeout->time, &now, remaining);
			
			eloop_remove_timeout(timeout);
			break;
		}
	}
	return removed;
}


int eloop_is_timeout_registered(eloop_timeout_handler handler,
				void *eloop_data, void *user_data)
{
	struct eloop_timeout *tmp;

	dl_list_for_each(tmp, &eloop.timeout, struct eloop_timeout, list) 
	{
		if (tmp->handler == handler && tmp->eloop_data == eloop_data && tmp->user_data == user_data)
			return 1;
	}

	return 0;
}


int eloop_deplete_timeout(unsigned int req_secs, unsigned int req_usecs,
			  eloop_timeout_handler handler, void *eloop_data, void *user_data)
{
	struct os_reltime now, requested, remaining;
	struct eloop_timeout *tmp;

	dl_list_for_each(tmp, &eloop.timeout, struct eloop_timeout, list) 
	{
		if (tmp->handler == handler && tmp->eloop_data == eloop_data && tmp->user_data == user_data) 
		{
			requested.sec = req_secs;
			requested.usec = req_usecs;
			os_get_reltime(&now);
			os_reltime_sub(&tmp->time, &now, &remaining);
			if (os_reltime_before(&requested, &remaining)) 
			{
				eloop_cancel_timeout(handler, eloop_data, user_data);
				eloop_register_timeout(requested.sec, requested.usec, handler, eloop_data, user_data);
				return 1;
			}
			return 0;
		}
	}

	return -1;
}


int eloop_replenish_timeout(unsigned int req_secs, unsigned int req_usecs,
			    eloop_timeout_handler handler, void *eloop_data, void *user_data)
{
	struct os_reltime now, requested, remaining;
	struct eloop_timeout *tmp;

	dl_list_for_each(tmp, &eloop.timeout, struct eloop_timeout, list)
	{
		if (tmp->handler == handler && tmp->eloop_data == eloop_data && tmp->user_data == user_data) 
		{
			requested.sec = req_secs;
			requested.usec = req_usecs;
			os_get_reltime(&now);
			os_reltime_sub(&tmp->time, &now, &remaining);
			if (os_reltime_before(&remaining, &requested)) 
			{
				eloop_cancel_timeout(handler, eloop_data, user_data);
				eloop_register_timeout(requested.sec,
						       requested.usec,
						       handler, eloop_data,
						       user_data);
				return 1;
			}
			return 0;
		}
	}

	return -1;
}


static void eloop_handle_alarm(int sig)
{
	printf("eloop: could not process SIGINT or SIGTERM in "
		   "two seconds. Looks like there\n"
		   "is a bug that ends up in a busy loop that "
		   "prevents clean shutdown.\n"
		   "Killing program forcefully.\n");
	exit(1);
}



static void eloop_handle_signal(int sig)
{
	int i;

	if ((sig == SIGINT || sig == SIGTERM) && !eloop.pending_terminate) 
	{
		/* Use SIGALRM to break out from potential busy loops that
		 * would not allow the program to be killed. */
		printf("!!!!! register signal SIGALRM for 2s\n");
		eloop.pending_terminate = 1;
		signal(SIGALRM, eloop_handle_alarm);
		alarm(2);
	}


	eloop.signaled++;
	for (i = 0; i < eloop.signal_count; i++) 
	{
		if (eloop.signals[i].sig == sig) 
		{
			eloop.signals[i].signaled++;
			break;
		}
	}
}


static void eloop_process_pending_signals(void)
{
	int i;

	if (eloop.signaled == 0)
		return;
	eloop.signaled = 0;

	if (eloop.pending_terminate) 
	{
		alarm(0);
		eloop.pending_terminate = 0;
	}

	for (i = 0; i < eloop.signal_count; i++) 
	{
		if (eloop.signals[i].signaled) 
		{
			eloop.signals[i].signaled = 0;
			eloop.signals[i].handler(eloop.signals[i].sig, eloop.signals[i].user_data);
		}
	}
}


int eloop_register_signal(int sig, eloop_signal_handler handler, void *user_data)
{
	struct eloop_signal *tmp;

	tmp = os_realloc_array(eloop.signals, eloop.signal_count + 1, sizeof(struct eloop_signal));
	if (tmp == NULL)
		return -1;

	tmp[eloop.signal_count].sig = sig;
	tmp[eloop.signal_count].user_data = user_data;
	tmp[eloop.signal_count].handler = handler;
	tmp[eloop.signal_count].signaled = 0;
	eloop.signal_count++;
	eloop.signals = tmp;
	signal(sig, eloop_handle_signal);

	return 0;
}


int eloop_register_signal_terminate(eloop_signal_handler handler, void *user_data)
{
	int ret = eloop_register_signal(SIGINT, handler, user_data);
	if (ret == 0)
		ret = eloop_register_signal(SIGTERM, handler, user_data);
	return ret;
}


int eloop_register_signal_reconfig(eloop_signal_handler handler, void *user_data)
{
	return eloop_register_signal(SIGHUP, handler, user_data);
}


void eloop_run(void)
{
	int timeout_ms = -1;

	int res;
	struct os_reltime tv, now;

	while (!eloop.terminate &&(!dl_list_empty(&eloop.timeout) || eloop.readers.count > 0 ||
		eloop.writers.count > 0 || eloop.exceptions.count > 0)) 
	{
		struct eloop_timeout *timeout;

		if (eloop.pending_terminate) 
		{
			/* 
			 * This may happen in some corner cases where a signal
			 * is received during a blocking operation. We need to
			 * process the pending signals and exit if requested to
			 * avoid hitting the SIGALRM limit if the blocking
			 * operation took more than two seconds.
			 */
			eloop_process_pending_signals();
			if (eloop.terminate)
				break;
		}

		timeout = dl_list_first(&eloop.timeout, struct eloop_timeout, list);
		if (timeout) 
		{
			os_get_reltime(&now);
			if (os_reltime_before(&now, &timeout->time))
				os_reltime_sub(&timeout->time, &now, &tv);
			else
				tv.sec = tv.usec = 0;

			timeout_ms = tv.sec * 1000 + tv.usec / 1000;

		}


		if (eloop.count == 0) 
		{
			res = 0;
		} 
		else 
		{
			res = epoll_wait(eloop.epollfd, eloop.epoll_events, eloop.count, timeout_ms);
		}

		if (res < 0 && errno != EINTR && errno != 0) 
		{
			printf("eloop: %s: %s", "epoll", strerror(errno));
			goto out;
		}

		eloop.readers.changed = 0;
		eloop.writers.changed = 0;
		eloop.exceptions.changed = 0;

		eloop_process_pending_signals();

		timeout = dl_list_first(&eloop.timeout, struct eloop_timeout, list);
		if (timeout) 
		{
			os_get_reltime(&now);
			if (!os_reltime_before(&now, &timeout->time)) 
			{
				void *eloop_data = timeout->eloop_data;
				void *user_data = timeout->user_data;
				eloop_timeout_handler handler = timeout->handler;
				eloop_remove_timeout(timeout);
				handler(eloop_data, user_data);
			}

		}

		if (res <= 0)
			continue;

		if (eloop.readers.changed || eloop.writers.changed || eloop.exceptions.changed) 
		{
			 /*
				在信号或超时处理程序中，套接字可能已被关闭并使用相同的fd重新打开，
				因此我们必须跳过前面的结果，再次检查当前注册的套接字是否有事件.
			  */
			continue;
		}

		eloop_sock_table_dispatch(eloop.epoll_events, res);

	}

	eloop.terminate = 0;
out:

	return;
}


void eloop_terminate(void)
{
	eloop.terminate = 1;
}


void eloop_destroy(void)
{
	struct eloop_timeout *timeout, *prev;
	struct os_reltime now;

	os_get_reltime(&now);
	dl_list_for_each_safe(timeout, prev, &eloop.timeout, struct eloop_timeout, list) 
	{
		int sec, usec;
		sec = timeout->time.sec - now.sec;
		usec = timeout->time.usec - now.usec;
		if (timeout->time.usec < now.usec) 
		{
			sec--;
			usec += 1000000;
		}
		printf("ELOOP: remaining timeout: %d.%06d "
			   "eloop_data=%p user_data=%p handler=%p", sec, usec, timeout->eloop_data, timeout->user_data, timeout->handler);
		eloop_remove_timeout(timeout);
	}
	eloop_sock_table_destroy(&eloop.readers);
	eloop_sock_table_destroy(&eloop.writers);
	eloop_sock_table_destroy(&eloop.exceptions);
	os_free(eloop.signals);

	os_free(eloop.fd_table);

	os_free(eloop.epoll_events);
	close(eloop.epollfd);
}


int eloop_terminated(void)
{
	return eloop.terminate || eloop.pending_terminate;
}


void eloop_wait_for_read_sock(int sock)
{
	/*
	 * We can use epoll() here. But epoll() requres 4 system calls.
	 * epoll_create1(), epoll_ctl() for ADD, epoll_wait, and close() for
	 * epoll fd. So select() is better for performance here.
	 */
	fd_set rfds;

	if (sock < 0)
		return;

	FD_ZERO(&rfds);
	FD_SET(sock, &rfds);
	select(sock + 1, &rfds, NULL, NULL, NULL);

}

