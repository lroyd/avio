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



#ifdef USE_SIMPLE_THREAD
	#include "uthread.h"
	#include "uepoll.h"
#endif


typedef struct _tagSampleAi
{
    HI_BOOL			bStart;
	HI_BOOL			bSaveFile;			//是否存成文件
    int				AiChn;
	int				s32QuitState;		//记录退出状态
    pthread_t		stAiPid;
	HI_VIDEO_CBK	pAiHandle;
	HI_VIDEO_CANCEL pAiCancel;			//用户取消点
	void			*pCancelArg;		//用户取消点参数				
}T_SampleAiInfo;

static T_SampleAiInfo	g_tSampleAi[HI_VIDEO_CHNNL_NUM];

#ifndef USE_SIMPLE_THREAD
static void *videoInputProcess(void *_pArg)
{
	int s32Ret, s32Fd;
    T_SampleAiInfo *pstPara;

    VENC_CHN s32Chnnl;
    VENC_CHN_STAT_S stStat;
    VENC_STREAM_S stStream;
    
    pstPara = (T_SampleAiInfo*)_pArg;
    s32Chnnl = pstPara->AiChn - 1;

    struct timeval stTimeout;
    fd_set read_fds;
	
	FILE *pFile;
	char szFilePostfix[10];
	HI_CHAR aszFileName[64];
	VENC_CHN_ATTR_S stVencChnAttr;
	PAYLOAD_TYPE_E enPayLoadType;
		
	if (pstPara->bSaveFile)
	{		
		s32Ret = HI_MPI_VENC_GetChnAttr(s32Chnnl, &stVencChnAttr);
		if(s32Ret != HI_SUCCESS)
		{
			printf("HI_MPI_VENC_GetChnAttr chn[%d] failed with %#x!\n", s32Chnnl + 1, s32Ret);
			return NULL;
		}
		enPayLoadType = stVencChnAttr.stVeAttr.enType;

		s32Ret = SAMPLE_COMM_VENC_GetFilePostfix(enPayLoadType, szFilePostfix);
		if(s32Ret != HI_SUCCESS)
		{
			printf("SAMPLE_COMM_VENC_GetFilePostfix [%d] failed with %#x!\n", stVencChnAttr.stVeAttr.enType, s32Ret);
			return NULL;
		}
		sprintf(aszFileName, "stream_chn%d%s", s32Chnnl + 1, szFilePostfix);
		pFile = fopen(aszFileName, "wb");
		if (!pFile)
		{
			printf("open file[%s] failed!\n", aszFileName);
			return NULL;
		}
		printf("[%d] save file %s\n", s32Chnnl + 1, aszFileName);		
	}
#if 0
#ifdef HI_VIDEO_VENC_SAVE_FILE_ON	
	FILE *pFile;
	char szFilePostfix[10];
	HI_CHAR aszFileName[64];
	VENC_CHN_ATTR_S stVencChnAttr;
	PAYLOAD_TYPE_E enPayLoadType;
	
	s32Ret = HI_MPI_VENC_GetChnAttr(s32Chnnl, &stVencChnAttr);
	if(s32Ret != HI_SUCCESS)
	{
		printf("HI_MPI_VENC_GetChnAttr chn[%d] failed with %#x!\n", s32Chnnl + 1, s32Ret);
		return NULL;
	}
	enPayLoadType = stVencChnAttr.stVeAttr.enType;

	s32Ret = SAMPLE_COMM_VENC_GetFilePostfix(enPayLoadType, szFilePostfix);
	if(s32Ret != HI_SUCCESS)
	{
		printf("SAMPLE_COMM_VENC_GetFilePostfix [%d] failed with %#x!\n", stVencChnAttr.stVeAttr.enType, s32Ret);
		return NULL;
	}
	sprintf(aszFileName, "stream_chn%d%s", s32Chnnl + 1, szFilePostfix);
	pFile = fopen(aszFileName, "wb");
	if (!pFile)
	{
		printf("open file[%s] failed!\n", aszFileName);
		return NULL;
	}
	printf("[%d] save file %s\n", s32Chnnl + 1, aszFileName);
#endif
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

        stTimeout.tv_sec  = 2;
        stTimeout.tv_usec = 0;
        s32Ret = select(s32Fd + 1, &read_fds, NULL, NULL, &stTimeout);
        if (s32Ret < 0)
        {
            printf("select failed!\n");
            break;
        }
        else if (s32Ret == 0)
        {
            printf("channel [%d] get venc stream time out, exit thread\n", s32Chnnl + 1);
            continue;
        }
        else
        {
			if (FD_ISSET(s32Fd, &read_fds))
			{
				memset(&stStream, 0, sizeof(stStream));
				
				if (HI_MPI_VENC_Query(s32Chnnl, &stStat))
				{
					printf("HI_MPI_VENC_Query chn[%d] failed with %#x!\n", s32Chnnl + 1, s32Ret);
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
				
				if (pstPara->bSaveFile)
				{
					if (SAMPLE_COMM_VENC_SaveStream(enPayLoadType, pFile, &stStream))
					{
						free(stStream.pstPack);
						stStream.pstPack = NULL;
						printf("save stream failed!\n");
						break;
					}					
				}
				else
				{
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
				}
#if 0			//哪个快？	
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
	if (pstPara->bSaveFile)
	{
		fclose(pFile);
	}
#if 0	
#ifdef HI_VIDEO_VENC_SAVE_FILE_ON	
    fclose(pFile);
#endif
#endif
    return NULL;
}

#else
static int videoInputProcess(void *_pArg)
{
	int s32Ret, s32Fd;
    T_SampleAiInfo *pstPara;
	
    VENC_CHN s32Chnnl;
    VENC_CHN_STAT_S stStat;
    VENC_STREAM_S stStream;
    
    pstPara = (T_SampleAiInfo*)_pArg;
    s32Chnnl = pstPara->AiChn - 1;
	
    struct timeval stTimeout;
    fd_set read_fds;	
	
	FILE *pFile;
	char szFilePostfix[10];
	HI_CHAR aszFileName[64];
	VENC_CHN_ATTR_S stVencChnAttr;
	PAYLOAD_TYPE_E enPayLoadType;	
	
	if (pstPara->bSaveFile)
	{		
		s32Ret = HI_MPI_VENC_GetChnAttr(s32Chnnl, &stVencChnAttr);
		if(s32Ret != HI_SUCCESS)
		{
			printf("HI_MPI_VENC_GetChnAttr chn[%d] failed with %#x!\n", s32Chnnl+ 1, s32Ret);
			return 0;
		}
		enPayLoadType = stVencChnAttr.stVeAttr.enType;

		s32Ret = SAMPLE_COMM_VENC_GetFilePostfix(enPayLoadType, szFilePostfix);
		if(s32Ret != HI_SUCCESS)
		{
			printf("SAMPLE_COMM_VENC_GetFilePostfix [%d] failed with %#x!\n", stVencChnAttr.stVeAttr.enType, s32Ret);
			return 0;
		}
		sprintf(aszFileName, "stream_chn%d%s", s32Chnnl+ 1, szFilePostfix);
		pFile = fopen(aszFileName, "wb");
		if (!pFile)
		{
			printf("open file[%s] failed!\n", aszFileName);
			return 0;
		}
		printf("[%d] save file %s\n", s32Chnnl+ 1, aszFileName);		
	}	
#if 0	
#ifdef HI_VIDEO_VENC_SAVE_FILE_ON
	FILE *pFile;
	char szFilePostfix[10];
	HI_CHAR aszFileName[64];
	VENC_CHN_ATTR_S stVencChnAttr;
	PAYLOAD_TYPE_E enPayLoadType;
	
	s32Ret = HI_MPI_VENC_GetChnAttr(s32Chnnl, &stVencChnAttr);
	if(s32Ret != HI_SUCCESS)
	{
		printf("HI_MPI_VENC_GetChnAttr chn[%d] failed with %#x!\n", s32Chnnl+ 1, s32Ret);
		return 0;
	}
	enPayLoadType = stVencChnAttr.stVeAttr.enType;

	s32Ret = SAMPLE_COMM_VENC_GetFilePostfix(enPayLoadType, szFilePostfix);
	if(s32Ret != HI_SUCCESS)
	{
		printf("SAMPLE_COMM_VENC_GetFilePostfix [%d] failed with %#x!\n", stVencChnAttr.stVeAttr.enType, s32Ret);
		return 0;
	}
	sprintf(aszFileName, "stream_chn%d%s", s32Chnnl+ 1, szFilePostfix);
	pFile = fopen(aszFileName, "wb");
	if (!pFile)
	{
		printf("open file[%s] failed!\n", aszFileName);
		return 0;
	}
	printf("[%d] save file %s\n", s32Chnnl+ 1, aszFileName);
#endif
#endif
	s32Fd = HI_MPI_VENC_GetFd(s32Chnnl);
	if (s32Fd < 0)
	{
		printf("HI_MPI_VENC_GetFd failed with %#x!\n", s32Fd);
		return 0;
	}
	
    while (pstPara->bStart)
    {
        FD_ZERO(&read_fds);
        FD_SET(s32Fd, &read_fds);

        stTimeout.tv_sec  = 2;
        stTimeout.tv_usec = 0;
        s32Ret = select(s32Fd + 1, &read_fds, NULL, NULL, &stTimeout);
        if (s32Ret < 0)
        {
            printf("select failed!\n");
            break;
        }
        else if (s32Ret == 0)
        {
            printf("channel [%d] get venc stream time out, exit thread\n", s32Chnnl + 1);
            continue;
        }
        else
        {
			if (FD_ISSET(s32Fd, &read_fds))
			{
				memset(&stStream, 0, sizeof(stStream));
				if (HI_MPI_VENC_Query(s32Chnnl, &stStat))
				{
					printf("HI_MPI_VENC_Query chn[%d] failed with %#x!\n", s32Chnnl+ 1, s32Ret);
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
				
				if (pstPara->bSaveFile)
				{
					if (SAMPLE_COMM_VENC_SaveStream(enPayLoadType, pFile, &stStream))
					{
						free(stStream.pstPack);
						stStream.pstPack = NULL;
						printf("save stream failed!\n");
						break;
					}					
				}
				else
				{
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
				}
#if 0				
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
	if (pstPara->bSaveFile)
	{
		fclose(pFile);
	}
#if 0	
#ifdef HI_VIDEO_VENC_SAVE_FILE_ON	
    fclose(pFile);
#endif
#endif
    return 0;
}


static void videoInputCleanup(void *_pArg, int _s32Code)
{
	T_SampleAiInfo *pstAi = (T_SampleAiInfo *)_pArg;
	//printf("NOTE: [%d] videoInputCleanup code %d !!\n", pstAi->AiChn, _s32Code);
}
	
#endif


HI_S32 SAMPLE_PROC_VPSS_StartGroup(VPSS_GRP VpssGrp, VPSS_GRP_ATTR_S *pstVpssGrpAttr, T_VpssCropInfo *pstGrpCrop)
{
    HI_S32 s32Ret;
    VPSS_NR_PARAM_U unNrParam = {{0}};
    
    if (VpssGrp < 0 || VpssGrp > VPSS_MAX_GRP_NUM)
    {
        printf("VpssGrp%d is out of rang. \n", VpssGrp);
        return HI_FAILURE;
    }

    if (HI_NULL == pstVpssGrpAttr)
    {
        printf("null ptr,line%d. \n", __LINE__);
        return HI_FAILURE;
    }

    s32Ret = HI_MPI_VPSS_CreateGrp(VpssGrp, pstVpssGrpAttr);
    if (s32Ret != HI_SUCCESS)
    {
        SAMPLE_PRT("HI_MPI_VPSS_CreateGrp failed with %#x!\n", s32Ret);
        return HI_FAILURE;
    }

    s32Ret = HI_MPI_VPSS_GetNRParam(VpssGrp, &unNrParam);
    if (s32Ret != HI_SUCCESS)
    {
        SAMPLE_PRT("failed with %#x!\n", s32Ret);
        return HI_FAILURE;
    }
    
    s32Ret = HI_MPI_VPSS_SetNRParam(VpssGrp, &unNrParam);
    if (s32Ret != HI_SUCCESS)
    {
        SAMPLE_PRT("failed with %#x!\n", s32Ret);
        return HI_FAILURE;
    }
	
	if (pstGrpCrop->m_bEnable)
	{
		VPSS_CROP_INFO_S stCropInfo;
		s32Ret = HI_MPI_VPSS_GetGrpCrop(VpssGrp, &stCropInfo);
		if(s32Ret != HI_SUCCESS)
		{
			SAMPLE_PRT("HI_MPI_VPSS_GetChnCrop failed with %#x!\n", s32Ret);
			return s32Ret;
		}
		
		stCropInfo.bEnable = 1;
		stCropInfo.enCropCoordinate = VPSS_CROP_ABS_COOR;
		stCropInfo.stCropRect.s32X = pstGrpCrop->m_u32X;
		stCropInfo.stCropRect.s32Y = pstGrpCrop->m_u32Y;
		stCropInfo.stCropRect.u32Width = pstGrpCrop->m_u32W;
		stCropInfo.stCropRect.u32Height = pstGrpCrop->m_u32H;
		
		s32Ret = HI_MPI_VPSS_SetGrpCrop(VpssGrp, &stCropInfo);
		if(s32Ret != HI_SUCCESS)
		{
			SAMPLE_PRT("HI_MPI_VPSS_SetGrpCrop failed with %#x!\n", s32Ret);
			return s32Ret;
		}	
	}


    s32Ret = HI_MPI_VPSS_StartGrp(VpssGrp);
    if (s32Ret != HI_SUCCESS)
    {
        SAMPLE_PRT("HI_MPI_VPSS_StartGrp failed with %#x\n", s32Ret);
        return HI_FAILURE;
    }

    return HI_SUCCESS;
}

int SAMPLE_PROC_VIDEO_CreatTrdAi(int _s32Chnnl, HI_VIDEO_CBK _pVideoCbk, HI_BOOL _bSaveFile)
{
	T_SampleAiInfo *pstAi = NULL;
	pstAi = &g_tSampleAi[_s32Chnnl - 1];
    pstAi->bStart= HI_TRUE;

    pstAi->AiChn = _s32Chnnl;
	pstAi->bSaveFile = _bSaveFile;
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
		//执行用户取消点
		pstAi->bStart = HI_FALSE;
		if (pstAi->pAiCancel)
		{
			pstAi->pAiCancel(pstAi->pCancelArg);
		}
        pthread_join(pstAi->stAiPid, 0);
    }

    return 0;
}

int SAMPLE_PROC_VIDEO_SetCancel(int _s32Chnnl, HI_VIDEO_CANCEL _pCancel, void *_pArg)
{
    T_SampleAiInfo	*pstAi = NULL;
    pstAi = &g_tSampleAi[_s32Chnnl - 1];	
	
	pstAi->pAiCancel	= _pCancel;
	pstAi->pCancelArg	= _pArg;
	
	return 0;
}



