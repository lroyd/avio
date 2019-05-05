/*************************************************************************
	> File Name: .h
	> Author: lroyd
	> Mail: htzhangxmu@163.com
	> Created Time: 
 ************************************************************************/
#ifndef __UTHREAD_H__
#define __UTHREAD_H__

#include <pthread.h>


//#define THR_SET_STACK_SIZE		(20*1024)	
//#define THR_MODE_DETACH_ON		


/************************************************************************
* Name: 		
* Descriptions:
* Parameter:		
* Return:     
* **********************************************************************/
int THR_EntityStart(int (*_pTask)(void *), void (*_pCleanup)(void *, int), void *_pArg, pthread_t *_pTid);









#endif


