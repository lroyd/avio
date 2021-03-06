/*************************************************************************
	> File Name: 
	> Author: lroyd
	> Mail: htzhangxmu@163.com
	> Created Time: 
 ************************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <pthread.h>
#include <errno.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#include "sample_comm.h"
#include "acodec.h"
#include "avio_api.h"
#include "config_site.h"


#ifdef USE_SIMPLE_THREAD
	#include "uthread.h"
#endif

#define AUDIO_IN_SAVE_FILE_NAME		("audio_in.pcm")
#define AUDIO_OUT_SAVE_FILE_NAME	("audio_out.pcm")

typedef struct _tagSampleAi
{
    HI_BOOL			bStart;
	HI_BOOL			bSaveFile;			//是否存成文件
    AUDIO_DEV		AiDev;
    int				AiChn;
	int				s32QuitState;		//记录退出状态
    pthread_t		stAiPid;
	HI_AUDIO_CBK	pAiHandle;
}T_SampleAiInfo;

typedef struct _tagSampleAo
{
	HI_BOOL			bStart;
	HI_BOOL			bSaveFile;			//是否存成文件
	AUDIO_DEV		AoDev;
	int				AoChn;
	pthread_t		stAoPid;
	HI_AUDIO_CBK	pAoHandle;
}T_SampleAoInfo;

#define SAMPLE_AUDIO_CHNNL_NO	(1)				//音频目前只有1路

static T_SampleAiInfo	g_tSampleAi[SAMPLE_AUDIO_CHNNL_NO];		
static T_SampleAoInfo	g_tSampleAo[SAMPLE_AUDIO_CHNNL_NO];


#ifndef USE_SIMPLE_THREAD
static void *audioCapture(void *_pArg)
{
    int s32Ret;
    int AiFd;
    T_SampleAiInfo *pstAiCtl = (T_SampleAiInfo *)_pArg;
    AUDIO_FRAME_S stFrame; 
	AEC_FRAME_S   stAecFrm;
    fd_set read_fds;
    struct timeval TimeoutVal;
    AI_CHN_PARAM_S stAiChnPara;

    if (HI_MPI_AI_GetChnParam(pstAiCtl->AiDev, pstAiCtl->AiChn, &stAiChnPara))
    {
        printf("%s: Get ai chn param failed\n", __FUNCTION__);
		
        return NULL;
    }

    stAiChnPara.u32UsrFrmDepth = 30;

    if (HI_MPI_AI_SetChnParam(pstAiCtl->AiDev, pstAiCtl->AiChn, &stAiChnPara))
    {
        printf("%s: set ai chn param failed\n", __FUNCTION__);
        return NULL;
    }
    
    FD_ZERO(&read_fds);
    AiFd = HI_MPI_AI_GetFd(pstAiCtl->AiDev, pstAiCtl->AiChn);
    FD_SET(AiFd,&read_fds);
	
	TimeoutVal.tv_sec = 1;
	TimeoutVal.tv_usec = 0;
	
	if (pstAiCtl->bSaveFile)
	{
		FILE    *pFile;
		pFile = fopen(AUDIO_IN_SAVE_FILE_NAME, "w+");		
	}
	
	printf("audioCapture start\n");
    while (pstAiCtl->bStart)
    {     

        FD_ZERO(&read_fds);
        FD_SET(AiFd,&read_fds);
        
        s32Ret = select(AiFd+1, &read_fds, NULL, NULL, &TimeoutVal);
        if (s32Ret < 0) 
        {
            break;
        }
        else if (0 == s32Ret) 
        {
            //printf("%s: get ai frame select time out\n", __FUNCTION__);
            continue;
        }
        
        if (FD_ISSET(AiFd, &read_fds))
        {
			memset(&stAecFrm, 0, sizeof(AEC_FRAME_S));
            if (HI_MPI_AI_GetFrame(pstAiCtl->AiDev, pstAiCtl->AiChn, &stFrame, &stAecFrm, HI_FALSE))
            {
				continue;
            }
			
			if (pstAiCtl->bSaveFile)
			{
				fwrite(stFrame.pVirAddr[0],  1 ,stFrame.u32Len , pFile);
				fflush(pFile);				
			}
			
			//回调用户
			//if (pstAiCtl->pAiHandle)
			{
				pstAiCtl->pAiHandle(stFrame.pVirAddr[0], stFrame.u32Len);
			}		

			
            if (HI_MPI_AI_ReleaseFrame(pstAiCtl->AiDev, pstAiCtl->AiChn, &stFrame, &stAecFrm))
            {
				//异常退出
                printf("%s: HI_MPI_AI_ReleaseFrame(%d, %d), failed with %#x!\n",__FUNCTION__, pstAiCtl->AiDev, pstAiCtl->AiChn, s32Ret);
				pstAiCtl->bStart = HI_FALSE;
				
                //return NULL;
            }
            
        }
    }
	
	if (pstAiCtl->bSaveFile) fclose(pFile);

    return NULL;
}


static void *audioPlay(void *_pArg)
{
    int s32Ret;

    T_SampleAoInfo *pstAoCtl = (T_SampleAoInfo *)_pArg;
    AUDIO_FRAME_S stFrame; 
	char play_buf[HI_AUDIO_PTNUMPERFRM * (HI_AUDIO_BIT_WIDTH + 1)] = {0};
	stFrame.pVirAddr[0] = play_buf;
	
	FILE    *pFile;
	if (pstAoCtl->bSaveFile)
	{
		pFile = fopen(AUDIO_OUT_SAVE_FILE_NAME, "w+");		
	}
	
	printf("audioPlay start\n");
    while (pstAoCtl->bStart)
    {
		//if (pstAoCtl->pAoHandle)
		{
			pstAoCtl->pAoHandle(stFrame.pVirAddr[0], stFrame.u32Len);
		}

		stFrame.u32Len = HI_AUDIO_PTNUMPERFRM * (HI_AUDIO_BIT_WIDTH + 1);
		stFrame.enBitwidth	= HI_AUDIO_BIT_WIDTH;
		stFrame.enSoundmode	= 0;
		
		if (pstAoCtl->bSaveFile)
		{
			fwrite(stFrame.pVirAddr[0],  1 ,stFrame.u32Len , pFile);
            fflush(pFile);			
		}
		
		if (HI_MPI_AO_SendFrame(pstAoCtl->AoDev, pstAoCtl->AoChn, &stFrame, 1000))
		{
			//异常退出
			printf("%s: HI_MPI_AO_SendFrame(%d, %d), failed with %#x!\n", __FUNCTION__, pstAoCtl->AoDev, pstAoCtl->AoChn, s32Ret);
			pstAoCtl->bStart = HI_FALSE;
			
			//return NULL;
		}    

    }
	
	if (pstAoCtl->bSaveFile) fclose(pFile);

    return NULL;
}


#else
static int audioCapture2(void *_pArg)
{
    int s32Ret;
    int AiFd;
    T_SampleAiInfo *pstAiCtl = (T_SampleAiInfo *)_pArg;
    AUDIO_FRAME_S stFrame; 
	AEC_FRAME_S   stAecFrm;
    fd_set read_fds;
    struct timeval TimeoutVal;
    AI_CHN_PARAM_S stAiChnPara;

    if (HI_MPI_AI_GetChnParam(pstAiCtl->AiDev, pstAiCtl->AiChn, &stAiChnPara))
    {
        printf("%s: Get ai chn param failed\n", __FUNCTION__);
		
        return 0;
    }

    stAiChnPara.u32UsrFrmDepth = 30;

    if (HI_MPI_AI_SetChnParam(pstAiCtl->AiDev, pstAiCtl->AiChn, &stAiChnPara))
    {
        printf("%s: set ai chn param failed\n", __FUNCTION__);
        return 0;
    }
    
    FD_ZERO(&read_fds);
    AiFd = HI_MPI_AI_GetFd(pstAiCtl->AiDev, pstAiCtl->AiChn);
    FD_SET(AiFd,&read_fds);
	
	TimeoutVal.tv_sec = 1;
	TimeoutVal.tv_usec = 0;
	
	FILE    *pFile;
	if (pstAiCtl->bSaveFile)
	{
		pFile = fopen(AUDIO_IN_SAVE_FILE_NAME, "w+");		
	}
	
	printf("audioCapture start\n");
    while (pstAiCtl->bStart)
    {     

        FD_ZERO(&read_fds);
        FD_SET(AiFd,&read_fds);
        
        s32Ret = select(AiFd+1, &read_fds, NULL, NULL, &TimeoutVal);
        if (s32Ret < 0) 
        {
            break;
        }
        else if (0 == s32Ret) 
        {
            //printf("%s: get ai frame select time out\n", __FUNCTION__);
            continue;
        }
        
        if (FD_ISSET(AiFd, &read_fds))
        {
			memset(&stAecFrm, 0, sizeof(AEC_FRAME_S));
            if (HI_MPI_AI_GetFrame(pstAiCtl->AiDev, pstAiCtl->AiChn, &stFrame, &stAecFrm, HI_FALSE))
            {
				continue;
            }
			
			if (pstAiCtl->bSaveFile)
			{
				fwrite(stFrame.pVirAddr[0],  1 ,stFrame.u32Len , pFile);
				fflush(pFile);				
			}
		
			//回调用户
			//if (pstAiCtl->pAiHandle)
			{
				pstAiCtl->pAiHandle(stFrame.pVirAddr[0], stFrame.u32Len);
			}		

			
            if (HI_MPI_AI_ReleaseFrame(pstAiCtl->AiDev, pstAiCtl->AiChn, &stFrame, &stAecFrm))
            {
				//异常退出
                printf("%s: HI_MPI_AI_ReleaseFrame(%d, %d), failed with %#x!\n",__FUNCTION__, pstAiCtl->AiDev, pstAiCtl->AiChn, s32Ret);
				pstAiCtl->bStart = HI_FALSE;
				
                //return NULL;
            }
            
        }
    }
	
	if (pstAiCtl->bSaveFile) fclose(pFile);

    return 0;
}

void audioCapCleanup2(void *_pArg, int _s32Code)
{
	printf("++++++++++++++audioCapCleanup2\n");
}

static int audioPlay2(void *_pArg)
{
    int s32Ret;

    T_SampleAoInfo *pstAoCtl = (T_SampleAoInfo *)_pArg;
    AUDIO_FRAME_S stFrame; 
	char play_buf[HI_AUDIO_PTNUMPERFRM * (HI_AUDIO_BIT_WIDTH + 1)] = {0};
	stFrame.pVirAddr[0] = play_buf;

	FILE    *pFile;
	if (pstAoCtl->bSaveFile)
	{
		pFile = fopen(AUDIO_OUT_SAVE_FILE_NAME, "w+");
	}
		
	printf("audioPlay start\n");
    while (pstAoCtl->bStart)
    {
		//if (pstAoCtl->pAoHandle)
		{
			pstAoCtl->pAoHandle(stFrame.pVirAddr[0], stFrame.u32Len);
		}

		stFrame.u32Len = HI_AUDIO_PTNUMPERFRM * (HI_AUDIO_BIT_WIDTH + 1);
		stFrame.enBitwidth	= HI_AUDIO_BIT_WIDTH;
		stFrame.enSoundmode	= 0;
		
		if (pstAoCtl->bSaveFile)
		{
			fwrite(stFrame.pVirAddr[0],  1 ,stFrame.u32Len , pFile);
            fflush(pFile);			
		}
		
		if (HI_MPI_AO_SendFrame(pstAoCtl->AoDev, pstAoCtl->AoChn, &stFrame, 1000))
		{
			//异常退出
			printf("%s: HI_MPI_AO_SendFrame(%d, %d), failed with %#x!\n", __FUNCTION__, pstAoCtl->AoDev, pstAoCtl->AoChn, s32Ret);
			pstAoCtl->bStart = HI_FALSE;
			
			//return NULL;
		}    

    }

	if (pstAoCtl->bSaveFile) fclose(pFile);

    return 0;
}

void audioPlayCleanup2(void *_pArg, int _s32Code)
{
	printf("++++++++++++++audioPlayCleanup2\n");
}
#endif



HI_S32 SAMPLE_PROC_AUDIO_StartAi(AUDIO_DEV AiDevId, HI_S32 s32AiChnCnt,
                                 AIO_ATTR_S* pstAioAttr, AUDIO_SAMPLE_RATE_E enOutSampleRate, HI_BOOL bResampleEn, HI_VOID* pstAiVqeAttr, HI_U32 u32AiVqeType,
								 HI_S32 _s32Volume)
{
    HI_S32 i;
    HI_S32 s32Ret;
	
    s32Ret = HI_MPI_AI_SetPubAttr(AiDevId, pstAioAttr);
    if (s32Ret)
    {
        printf("%s: HI_MPI_AI_SetPubAttr(%d) failed with %#x\n", __FUNCTION__, AiDevId, s32Ret);
        return s32Ret;
    }
	
    s32Ret = HI_MPI_AI_Enable(AiDevId);
	if (s32Ret)
    {
        printf("%s: HI_MPI_AI_Enable(%d) failed with %#x\n", __FUNCTION__, AiDevId, s32Ret);
        return s32Ret;
    }   
	
    for (i=0; i<s32AiChnCnt; i++)
    {
        s32Ret = HI_MPI_AI_EnableChn(AiDevId, i/(pstAioAttr->enSoundmode + 1));
        if (s32Ret)
        {
            printf("%s: HI_MPI_AI_EnableChn(%d,%d) failed with %#x\n", __FUNCTION__, AiDevId, i, s32Ret);
            return s32Ret;
        }        

        if (HI_TRUE == bResampleEn)
        {
            s32Ret = HI_MPI_AI_EnableReSmp(AiDevId, i, enOutSampleRate);
			if (s32Ret)
            {
                printf("%s: HI_MPI_AI_EnableReSmp(%d,%d) failed with %#x\n", __FUNCTION__, AiDevId, i, s32Ret);
                return s32Ret;
            }
        }

		if (NULL != pstAiVqeAttr)
        {
			HI_BOOL bAiVqe = HI_TRUE;
			switch (u32AiVqeType)
            {
				case 0:
                    s32Ret = HI_SUCCESS;
					bAiVqe = HI_FALSE;
                    break;
                case 1:
                    s32Ret = HI_MPI_AI_SetVqeAttr(AiDevId, i, SAMPLE_AUDIO_AO_DEV, i, (AI_VQE_CONFIG_S *)pstAiVqeAttr);
                    break;
                default:
                    s32Ret = HI_FAILURE;
                    break;
            }
			if (s32Ret)
		    {
                printf("%s: SetAiVqe%d(%d,%d) failed with %#x\n", __FUNCTION__, u32AiVqeType, AiDevId, i, s32Ret);
		        return s32Ret;
		    }
			
		    if (bAiVqe)
            {
				s32Ret = HI_MPI_AI_EnableVqe(AiDevId, i);
				if (s32Ret)
				{
					printf("%s: HI_MPI_AI_EnableVqe(%d,%d) failed with %#x\n", __FUNCTION__, AiDevId, i, s32Ret);
					return s32Ret;
				}
			}
			
			HI_MPI_AI_SetVqeVolume(AiDevId, i, _s32Volume);
		}
    }
    
    return HI_SUCCESS;
}


int SAMPLE_PROC_AUDIO_CreatTrdAi(AUDIO_DEV AiDev, AI_CHN AiChn, HI_AUDIO_CBK _pAudioCap, HI_BOOL _bSaveFile)
{
	T_SampleAiInfo *pstAi = NULL;
	pstAi = &g_tSampleAi[0];
    pstAi->bStart= HI_TRUE;
	pstAi->bSaveFile = _bSaveFile;
    pstAi->AiDev = AiDev;
    pstAi->AiChn = AiChn;
	pstAi->pAiHandle = _pAudioCap;

#ifndef USE_SIMPLE_THREAD	
	pthread_create(&pstAi->stAiPid, 0, audioCapture, pstAi);
#else    
	THR_EntityStart(audioCapture2, audioCapCleanup2, (void *)pstAi, &pstAi->stAiPid);
#endif	
    return 0;
}

int SAMPLE_PROC_AUDIO_CreatTrdAo(AUDIO_DEV AoDev, AO_CHN AoChn, HI_AUDIO_CBK _pAudioPlay, HI_BOOL _bSaveFile)
{
	T_SampleAoInfo	*pstAo = NULL;
	pstAo = &g_tSampleAo[0];
    pstAo->bStart= HI_TRUE;
	pstAo->bSaveFile = _bSaveFile;
    pstAo->AoDev = AoDev;
    pstAo->AoChn = AoChn;	
	pstAo->pAoHandle = _pAudioPlay;	
#ifndef USE_SIMPLE_THREAD	
	pthread_create(&pstAo->stAoPid, 0, audioPlay, pstAo);
#else
	THR_EntityStart(audioPlay2, audioPlayCleanup2, pstAo, &pstAo->stAoPid);
#endif	
	
    return 0;
}

int SAMPLE_PROC_AUDIO_Play(char *pData, int _s32Len)
{
	AUDIO_FRAME_S stFrame; 
	stFrame.pVirAddr[0] = pData;
	stFrame.u32Len = _s32Len; 
	stFrame.enBitwidth	= HI_AUDIO_BIT_WIDTH;
	stFrame.enSoundmode	= 0;
	if (HI_MPI_AO_SendFrame(0, 0, &stFrame, 1000))
	{
		printf("%s: HI_MPI_AO_SendFrame(%d, %d), failed with %#x!",__FUNCTION__, 0, 0);
	}	
	
	return 0;
}

///////////////////////////////////////////////////////////////////////////////////////
int SAMPLE_PROC_AUDIO_DestoryTrdAi(AUDIO_DEV AiDev, AI_CHN AiChn)
{
    T_SampleAiInfo	*pstAi = NULL;
    pstAi = &g_tSampleAi[0];

    if (pstAi->bStart)
    {
        pstAi->bStart = HI_FALSE;
        pthread_join(pstAi->stAiPid, 0);
    }

    return 0;
}

int SAMPLE_PROC_AUDIO_DestoryTrdAo(AUDIO_DEV AoDev, AO_CHN AoChn)
{
    T_SampleAoInfo	*pstAo = NULL;
	pstAo = &g_tSampleAo[0];

 
	if (pstAo->bStart)
    {
        pstAo->bStart = HI_FALSE;
        pthread_join(pstAo->stAoPid, 0);
    }

    return 0;
}
