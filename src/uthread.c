/*************************************************************************
	> File Name: .c
	> Author: lroyd
	> Mail: htzhangxmu@163.com
	> Created Time: 
*************************************************************************/
#include <stdarg.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/time.h>
#include <sys/types.h>
#include <assert.h>

#include "uthread.h"

/************************************************************************
* 			LOG	Setting
************************************************************************/
#define LOG_TAG_STR			("THR")
#define LOG_TAG_LEVE		(1)

#define LOG_ON				(0)
#define LOG_OFF				(1)

#define LOGOUT(level, format, arg...)	if(level < LOG_TAG_LEVE){printf("[%s]"format"", LOG_TAG_STR, ##arg);printf("\r\n");}


#define THR_WAIT_TASK			((void *)(-2))

typedef struct _tagThread
{
	pthread_t m_tTid;			
	int		  m_tQuitCode;		
	int (*Task)(void *);					
	void (*Cleanup)(void *, int);
	void *m_pArg;							

} LH_Thread;

/************************************************************************
* Name: 		
* Descriptions:
* Parameter:		
* Return:     
* **********************************************************************/
static void XSleep(int _nSec, int _nUSec)
{
	struct timeval tv;
	tv.tv_sec = _nSec;
	tv.tv_usec = _nUSec;
	select(0, NULL, NULL, NULL, &tv);
}
/************************************************************************
* Name: 		
* Descriptions:
* Parameter:		
* Return:     
* **********************************************************************/
static void exceptThread(void *_pArg)  
{
	LH_Thread *ptThr = (LH_Thread *)_pArg;
    //LOGOUT(LOG_ON, "except thread quit code = %d", ptThr->m_tQuitCode); 
	
	if (ptThr->Cleanup)
	{
		ptThr->Cleanup(ptThr->m_pArg, ptThr->m_tQuitCode);
	}
	
	free(ptThr);
}
/************************************************************************
* Name: 		
* Descriptions:
* Parameter:		
* Return:     
* **********************************************************************/
static void *interThread(void *_pArg)
{
	LH_Thread *ptThr = (LH_Thread *)_pArg;
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
	pthread_cleanup_push(exceptThread, _pArg);
	ptThr->m_tQuitCode = -1;
	while (THR_WAIT_TASK == ptThr->Task)
	{
		XSleep(0,1000);
	}
	ptThr->m_tQuitCode = ptThr->Task(ptThr->m_pArg);
	pthread_cleanup_pop(1);
	return NULL;
}

/************************************************************************
* Name: 		
* Descriptions:
* Parameter:		
* Return:     
* **********************************************************************/
int THR_EntityStart(int (*_pTask)(void *), void (*_pCleanup)(void *, int), void *_pArg, pthread_t *_pTid)
{
	LH_Thread *ptThr = malloc(sizeof(LH_Thread));
	if (ptThr == NULL)
	{
		LOGOUT(LOG_ON, "malloc error");
		return -1;
	}
	
	pthread_t tTid;
#ifdef THR_SET_STACK_SIZE	
	pthread_attr_t iThrAttr;
	pthread_attr_init(&iThrAttr);
	if(pthread_attr_setstacksize(&iThrAttr, THR_SET_STACK_SIZE)) 
	{
		pthread_attr_destroy(&iThrAttr);
		LOGOUT(LOG_ON, "pthread_attr_setstacksize error");
		goto EXIT;
	}
#endif	
	ptThr->m_pArg = _pArg;
	ptThr->Task = THR_WAIT_TASK; 
#ifdef THR_SET_STACK_SIZE	
	if (pthread_create(&tTid, &iThrAttr, interThread, ptThr) != 0)
#else		
	if (pthread_create(&tTid, NULL, interThread, ptThr) != 0)
#endif		
	{
		LOGOUT(LOG_ON, "pthread_create error");
		goto EXIT;
	}
	ptThr->m_tTid	= tTid;
	ptThr->m_pArg	= _pArg;
	ptThr->Task		= _pTask;		
	ptThr->Cleanup	= _pCleanup;
	
#ifdef THR_SET_STACK_SIZE	
	pthread_attr_destroy(&iThrAttr);
#endif	
#ifdef THR_MODE_DETACH_ON
	pthread_detach(tTid);
#endif
	//LOGOUT(LOG_ON, "new task thread pid = (int)%d , = %p", (int)tTid, ptThr->Task );
	
	if (_pTid) *_pTid = tTid;	
	return 0;
	
EXIT:	
	if (ptThr) free(ptThr);
	
	return -1;
}


