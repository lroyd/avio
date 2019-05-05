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
#include "avio_api.h"
#include "config_site.h"


//#define USE_SIMPLE_THREAD
#ifdef USE_SIMPLE_THREAD
	#include "uthread.h"
	#include "uepoll.h"
#endif


typedef struct _tagSampleAi
{
    HI_BOOL			bStart;
    int				AiChn;
	int				s32QuitState;		//记录退出状态
    pthread_t		stAiPid;
	HI_VIDEO_CBK	pAiHandle;
}T_SampleAiInfo;

static T_SampleAiInfo	g_tSampleAi[HI_VIDEO_CHNNL_NUM];

#ifndef USE_SIMPLE_THREAD
static void *videoInputProcess(void *_pArg)
{
	int s32Ret, s32Fd;
    
    T_SampleAiInfo *pstPara;

    struct timeval TimeoutVal;
    fd_set read_fds;

	
    VENC_CHN s32Chnnl;
    VENC_CHN_STAT_S stStat;
    VENC_STREAM_S stStream;
    
    
    pstPara = (T_SampleAiInfo*)_pArg;
    s32Chnnl = pstPara->AiChn - 1;
	
#ifdef HI_VIDEO_VENC_SAVE_FILE_ON	
	FILE *pFile;
	char szFilePostfix[10];
	HI_CHAR aszFileName[64];
	VENC_CHN_ATTR_S stVencChnAttr;
	PAYLOAD_TYPE_E enPayLoadType;
	
	s32Ret = HI_MPI_VENC_GetChnAttr(s32Chnnl, &stVencChnAttr);
	if(s32Ret != HI_SUCCESS)
	{
		printf("HI_MPI_VENC_GetChnAttr chn[%d] failed with %#x!\n", s32Chnnl, s32Ret);
		return NULL;
	}
	enPayLoadType = stVencChnAttr.stVeAttr.enType;

	s32Ret = SAMPLE_COMM_VENC_GetFilePostfix(enPayLoadType, szFilePostfix);
	if(s32Ret != HI_SUCCESS)
	{
		printf("SAMPLE_COMM_VENC_GetFilePostfix [%d] failed with %#x!\n", stVencChnAttr.stVeAttr.enType, s32Ret);
		return NULL;
	}
	sprintf(aszFileName, "stream_chn%d%s", s32Chnnl, szFilePostfix);
	pFile = fopen(aszFileName, "wb");
	if (!pFile)
	{
		printf("open file[%s] failed!\n", aszFileName);
		return NULL;
	}
	printf("[%d] save file %s\n", s32Chnnl, aszFileName);
#endif

	s32Fd = HI_MPI_VENC_GetFd(s32Chnnl);
	if (s32Fd < 0)
	{
		printf("HI_MPI_VENC_GetFd failed with %#x!\n", s32Fd);
		return NULL;
	}

    while (pstPara->bStart)
    {
        FD_ZERO(&read_fds);
        FD_SET(s32Fd, &read_fds);

        TimeoutVal.tv_sec  = 2;
        TimeoutVal.tv_usec = 0;
        s32Ret = select(s32Fd + 1, &read_fds, NULL, NULL, &TimeoutVal);
        if (s32Ret < 0)
        {
            printf("select failed!\n");
            break;
        }
        else if (s32Ret == 0)
        {
            printf("channel [%d] get venc stream time out, exit thread\n", s32Chnnl);
            continue;
        }
        else
        {
			if (FD_ISSET(s32Fd, &read_fds))
			{
				memset(&stStream, 0, sizeof(stStream));
				
				if (HI_MPI_VENC_Query(s32Chnnl, &stStat))
				{
					printf("HI_MPI_VENC_Query chn[%d] failed with %#x!\n", s32Chnnl, s32Ret);
					break;
				}
				
				if(0 == stStat.u32CurPacks)
				{
					  printf("NOTE: Current  frame is NULL!\n");
					  continue;
				}

				stStream.pstPack = (VENC_PACK_S*)malloc(sizeof(VENC_PACK_S) * stStat.u32CurPacks);
				if (NULL == stStream.pstPack)
				{
					printf("malloc stream pack failed!\n");
					break;
				}

				stStream.u32PackCount = stStat.u32CurPacks;
				if (HI_MPI_VENC_GetStream(s32Chnnl, &stStream, HI_TRUE))
				{
					free(stStream.pstPack);
					stStream.pstPack = NULL;
					printf("HI_MPI_VENC_GetStream failed with %#x!\n", s32Ret);
					break;
				}
				
#ifdef HI_VIDEO_VENC_SAVE_FILE_ON							
				if (SAMPLE_COMM_VENC_SaveStream(enPayLoadType, pFile, &stStream))
				{
					free(stStream.pstPack);
					stStream.pstPack = NULL;
					printf("save stream failed!\n");
					break;
				}
#else
				int i = 0;
				for (i= 0; i<stStream.u32PackCount; i++)
				{
					//if (pstPara->pAiHandle)
					{
						pstPara->pAiHandle(stStream.pstPack[i].DataType.enH264EType,\
								stStream.pstPack[i].pu8Addr+stStream.pstPack[i].u32Offset,\
								stStream.pstPack[i].u32Len-stStream.pstPack[i].u32Offset, \
								stStream.pstPack[i].u64PTS);
					}
				}
#endif						

				if (HI_MPI_VENC_ReleaseStream(s32Chnnl, &stStream))
				{
					free(stStream.pstPack);
					stStream.pstPack = NULL;
					break;
				}

				free(stStream.pstPack);
				stStream.pstPack = NULL;
			}

        }
    }
#ifdef HI_VIDEO_VENC_SAVE_FILE_ON	
    fclose(pFile);
#endif
    return NULL;
}

#else
static int videoInputProcess(void *_pArg)
{
	int s32Ret, s32Fd;
    int s32Epfd, nfd;
	struct epoll_event events[8];
	
    T_SampleAiInfo *pstPara;
	
    VENC_CHN s32Chnnl;
    VENC_CHN_STAT_S stStat;
    VENC_STREAM_S stStream;
    
    pstPara = (T_SampleAiInfo*)_pArg;
    s32Chnnl = pstPara->AiChn - 1;
	
#ifdef HI_VIDEO_VENC_SAVE_FILE_ON	
	FILE *pFile;
	char szFilePostfix[10];
	HI_CHAR aszFileName[64];
	VENC_CHN_ATTR_S stVencChnAttr;
	PAYLOAD_TYPE_E enPayLoadType;
	
	s32Ret = HI_MPI_VENC_GetChnAttr(s32Chnnl, &stVencChnAttr);
	if(s32Ret != HI_SUCCESS)
	{
		printf("HI_MPI_VENC_GetChnAttr chn[%d] failed with %#x!\n", s32Chnnl, s32Ret);
		return 0;
	}
	enPayLoadType = stVencChnAttr.stVeAttr.enType;

	s32Ret = SAMPLE_COMM_VENC_GetFilePostfix(enPayLoadType, szFilePostfix);
	if(s32Ret != HI_SUCCESS)
	{
		printf("SAMPLE_COMM_VENC_GetFilePostfix [%d] failed with %#x!\n", stVencChnAttr.stVeAttr.enType, s32Ret);
		return 0;
	}
	sprintf(aszFileName, "stream_chn%d%s", s32Chnnl, szFilePostfix);
	pFile = fopen(aszFileName, "wb");
	if (!pFile)
	{
		printf("open file[%s] failed!\n", aszFileName);
		return 0;
	}
	printf("[%d] save file %s\n", s32Chnnl, aszFileName);
#endif

	UEPOLL_Create(&s32Epfd);

	s32Fd = HI_MPI_VENC_GetFd(s32Chnnl);
	if (s32Fd < 0)
	{
		printf("HI_MPI_VENC_GetFd failed with %#x!\n", s32Fd);
		return 0;
	}

	UEPOLL_Add(s32Epfd, s32Fd);
	
    while (pstPara->bStart)
    {
		nfd = epoll_wait(s32Epfd, events, 8, 2000);  //2s
		if (nfd <= 0)  
		{
			printf("channel [%d] get venc stream time out, exit thread\n", s32Chnnl);
			continue;
		}
        else
        {
			memset(&stStream, 0, sizeof(stStream));
			if (HI_MPI_VENC_Query(s32Chnnl, &stStat))
			{
				printf("HI_MPI_VENC_Query chn[%d] failed with %#x!\n", s32Chnnl, s32Ret);
				break;
			}
			
			if(0 == stStat.u32CurPacks)
			{
				  printf("NOTE: Current  frame is NULL!\n");
				  continue;
			}

			stStream.pstPack = (VENC_PACK_S*)malloc(sizeof(VENC_PACK_S) * stStat.u32CurPacks);
			if (NULL == stStream.pstPack)
			{
				printf("malloc stream pack failed!\n");
				break;
			}

			stStream.u32PackCount = stStat.u32CurPacks;
			if (HI_MPI_VENC_GetStream(s32Chnnl, &stStream, HI_TRUE))
			{
				free(stStream.pstPack);
				stStream.pstPack = NULL;
				printf("HI_MPI_VENC_GetStream failed with %#x!\n", s32Ret);
				break;
			}

#ifdef HI_VIDEO_VENC_SAVE_FILE_ON							
			if (SAMPLE_COMM_VENC_SaveStream(enPayLoadType, pFile, &stStream))
			{
				free(stStream.pstPack);
				stStream.pstPack = NULL;
				printf("save stream failed!\n");
				break;
			}
#else
			int i = 0;
			for (i= 0; i<stStream.u32PackCount; i++)
			{
				//if (pstPara->pAiHandle)
				{
					pstPara->pAiHandle(stStream.pstPack[i].DataType.enH264EType,\
							stStream.pstPack[i].pu8Addr+stStream.pstPack[i].u32Offset,\
							stStream.pstPack[i].u32Len-stStream.pstPack[i].u32Offset, \
							stStream.pstPack[i].u64PTS);
				}
			}
#endif					

			if (HI_MPI_VENC_ReleaseStream(s32Chnnl, &stStream))
			{
				free(stStream.pstPack);
				stStream.pstPack = NULL;
				break;
			}

			free(stStream.pstPack);
			stStream.pstPack = NULL;
        }
    }
#ifdef HI_VIDEO_VENC_SAVE_FILE_ON	
    fclose(pFile);
#endif
    return 0;
}


static void videoInputCleanup(void *_pArg, int _s32Code)
{
	T_SampleAiInfo *pstAi = (T_SampleAiInfo *)_pArg;
	printf("NOTE: [%d] videoInputCleanup code %d !!\n", pstAi->AiChn, _s32Code);
}
	
#endif



int SAMPLE_PROC_VIDEO_CreatTrdAi(int _s32Chnnl, HI_VIDEO_CBK _pVideoCbk)
{
	T_SampleAiInfo *pstAi = NULL;
	pstAi = &g_tSampleAi[_s32Chnnl - 1];
    pstAi->bStart= HI_TRUE;

    pstAi->AiChn = _s32Chnnl;
	pstAi->pAiHandle = _pVideoCbk;	
	
#ifndef USE_SIMPLE_THREAD
    pthread_create(&pstAi->stAiPid, 0, videoInputProcess, pstAi);
#else
	THR_EntityStart(videoInputProcess, videoInputCleanup, pstAi, &pstAi->stAiPid);
#endif	

    return 0;
}


int SAMPLE_PROC_VIDEO_DestoryTrdAi(int _s32Chnnl)
{
    T_SampleAiInfo	*pstAi = NULL;
    pstAi = &g_tSampleAi[_s32Chnnl - 1];

    if (pstAi->bStart)
    {
        pstAi->bStart = HI_FALSE;
        pthread_join(pstAi->stAiPid, 0);
    }

    return 0;
}
