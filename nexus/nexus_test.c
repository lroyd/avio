#include <stdio.h>
#include <string.h>


#include "pylon.h"
#include "nexus.h"

#define USR_TIMEOUT	(2)

struct os_time t1,t2, t;

void timeout(void *e, void *u)
{
	os_get_time(&t2);
	os_time_sub(&t2, &t1, &t);
	printf("timeout sec = %ld, usec = %ld\n", t.sec, t.usec);
	os_get_time(&t1);
	eloop_register_timeout(USR_TIMEOUT, 0, timeout, NULL, NULL);
	while(1)
	{
		sleep(2);
		printf("you are die\n");
	}
}


static void terminate(int sig, void *ctx)
{
	eloop_terminate();
}

int main(void)
{
	
	eloop_init();
	
	os_get_time(&t1);
	eloop_register_timeout(USR_TIMEOUT, 0, timeout, NULL, NULL);
	
	
	eloop_register_signal_terminate(terminate, NULL);
	eloop_run();
	
	eloop_destroy();
    
	return 0;
}
