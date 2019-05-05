/*******************************************************************************
	> File Name: 
	> Author: lroyd
	> Mail: htzhangxmu@163.com
	> Created Time: 
 *******************************************************************************/
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>

#include <sys/time.h>

#include "uepoll.h"
#include "common.h"
//#include "ulogfile.h"

/************************************************************************
* 			LOG	Setting
************************************************************************/
#define LOG_TAG_STR			("EP")
#define LOG_TAG_LEVE		LOG_WARNING

#define LOGOUT(level, format, arg...)	if(level < LOG_TAG_LEVE){printf("[%s]"format"", LOG_TAG_STR, ##arg);printf("\r\n");}

/************************************************************************
* Name: 		
* Descriptions:
* Parameter:		
* Return:     
* **********************************************************************/
int
UEPOLL_Create(int *_pOutEfd)
{
	*_pOutEfd = -1;
	*_pOutEfd = epoll_create(EPOLL_EVENT_NUM_MAX); 
	if(*_pOutEfd < 0)
	{
		LOGOUT(LOG_ERROR, "epoll_create error");
		return -1;		
	}	

	return 0;
}

/************************************************************************
* Name: 		
* Descriptions:
* Parameter:		
* Return:     
* **********************************************************************/
int
UEPOLL_Add(int _pEfd, int _pFd)
{
	_ERROR_RETURN(_pEfd >=0 && _pFd >= 0, -1);
	
	struct epoll_event ev;
	ev.data.fd = _pFd;
	ev.events = (EPOLLIN|EPOLLET);
	
	if(epoll_ctl(_pEfd, EPOLL_CTL_ADD, _pFd, &ev) == -1)
	{
		LOGOUT(LOG_ERROR, "epoll_ctl add error");
		return -1;
	}
	
	return 0;
}
/************************************************************************
* Name: 		
* Descriptions:
* Parameter:		
* Return:     
* **********************************************************************/
int
UEPOLL_Del(int _pEfd, int _pFd) 
{
	_ERROR_RETURN(_pEfd >=0 && _pFd >= 0, -1);
	
	if(epoll_ctl(_pEfd, EPOLL_CTL_DEL, _pFd, NULL) == -1)
	{
		LOGOUT(LOG_ERROR, "epoll_ctl del error");
		return -1;
	}
	
	return 0;
}


