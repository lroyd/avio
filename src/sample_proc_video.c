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

#include "list.h"
#include "avio_api.h"
#include "config_site.h"



#ifdef USE_SIMPLE_THREAD
	#include "uthread.h"
	#include "uepoll.h"
#endif


#define VIDEO_CHANNEL_SERVER_MAX	(4)	//每个通道最多支持4个服务

typedef struct _tagServerNode
{
    _DECL_LIST_MEMBER(struct _tagServerNode);
	const char		*m_pName;	//目前没有
	VIDEO_SERVER_TYPE	m_emType;
    HI_VIDEO_CBK	m_pHandle;
	HI_VIDEO_CANCEL m_pCancel;		//每一个server都应该有一个取消点
	void			*pCancelArg;	//取消点参数		
} T_ServerNode;

typedef struct _tagViServer
{
	T_ServerNode	in_tList;		//服务链表头
	int				m_u32Cnt;		//当前服务个数
}T_ViServerInfo;

typedef struct _tagSampleAi
{
    HI_BOOL			bStart;
    int				AiChn;		//1+
	int				s32QuitState;		//记录退出状态
    pthread_t		stAiPid;
	
	T_ViServerInfo	in_tServer;
	
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
		
	if (0)
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
				
				if (0)
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
	if (0)
	{
		fclose(pFile);
	}

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
	
	if (0)
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

				int i = 0;
				for (i= 0; i<stStream.u32PackCount; i++)
				{
#if 0					
					//if (pstPara->pAiHandle)
					{
						pstPara->pAiHandle(stStream.pstPack[i].DataType.enH264EType,\
								stStream.pstPack[i].pu8Addr+stStream.pstPack[i].u32Offset,\
								stStream.pstPack[i].u32Len-stStream.pstPack[i].u32Offset, \
								stStream.pstPack[i].u64PTS);
					}
#endif					
				}
				

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
	if (0)
	{
		fclose(pFile);
	}

    return 0;
}


static void videoInputCleanup(void *_pArg, int _s32Code)
{
	T_SampleAiInfo *pstAi = (T_SampleAiInfo *)_pArg;
	printf("NOTE: [%d] videoInputCleanup code %d !!\n", pstAi->AiChn, _s32Code);
}
	
#endif

//检查是否可以添加0：不可以添加	1：可以添加
static int serverListCheck(T_ViServerInfo *_pList)
{
	return (NF_ListGetTotal(_pList) >= VIDEO_CHANNEL_SERVER_MAX? 0: 1);
}

static void serverListClean(T_ViServerInfo *_pList)
{
	int i, u32NodeNum = 0; 
	T_ViServerInfo *pList = _pList;
	pList->m_u32Cnt = 0;
	u32NodeNum = NF_ListGetTotal(pList);
	if (u32NodeNum)
	{
		//不为0，还有节点需要删除
		for (i = 0; i < u32NodeNum; i++) 
		{
			//NF_ListNodeDelete(&nodes[i]);
		}
	}	
}

//后向追加
static void serverListAdd(T_ViServerInfo *_pList, T_ServerNode *_pNode)
{
	NF_ListInsertAppend(_pList, _pNode);
}


int SAMPLE_PROC_VPSS_StartGroup(VPSS_GRP VpssGrp, VPSS_GRP_ATTR_S *pstVpssGrpAttr, T_VpssCropInfo *pstGrpCrop)
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


void SAMPLE_PROC_VIDEO_Init(void)
{
	int i, j, u32NodeNum = 0;
	T_SampleAiInfo *pstAi = NULL;
	for (i = 0; i < HI_VIDEO_CHNNL_NUM; i++)
	{
		pstAi = &g_tSampleAi[i];
		pstAi->bStart	= HI_FALSE;
		pstAi->AiChn	= 0;
		pstAi->s32QuitState	= 0;
		
		serverListClean(&(pstAi->in_tServer.in_tList));
		
	}
	
	
	
}


//目前不支持动态注册，后续添加
int SAMPLE_PROC_VIDEO_RegisterServer(int _s32Chnnl, VIDEO_SERVER_TYPE _emType, HI_VIDEO_CBK _pHandle, HI_VIDEO_CANCEL _pCancel, void *_pArg)
{
	int s32Ret = -1;
	T_SampleAiInfo *pstAi = NULL;
	T_ViServerInfo *pList = NULL;
	pstAi = &g_tSampleAi[_s32Chnnl - 1];	
	pstAi->AiChn = _s32Chnnl;
	
	pList = &(pstAi->in_tServer.in_tList);
	//找到对应的链表
	if (serverListCheck(pList))
	{
		T_ServerNode *pNode = (T_ServerNode *)malloc(sizeof(T_ServerNode));
		if (!pNode)	return s32Ret;
		
		pNode->m_emType		= _emType;
		pNode->m_pHandle	= _pHandle;
		pNode->m_pCancel	= _pCancel;
		pNode->pCancelArg	= _pArg;
		serverListAdd(pList, pNode);
		s32Ret = 0;
	}
	
	return s32Ret;
}

int SAMPLE_PROC_VIDEO_CreatTrdAi(int _s32Chnnl)
{
	T_SampleAiInfo *pstAi = NULL;
	pstAi = &g_tSampleAi[_s32Chnnl - 1];
	
	if (pstAi->AiChn == 0)
	{
		printf("please first set server(use HI_AVIO_VideoRegisterServer) for channel[%d]\n", _s32Chnnl);
		goto EXIT;
	}
	
	if (pstAi->bStart)
	{
		printf("video channel[%d] has start\n", _s32Chnnl);
		goto EXIT;		
	}
	
    pstAi->bStart = HI_TRUE;
	
#ifndef USE_SIMPLE_THREAD
    pthread_create(&pstAi->stAiPid, 0, videoInputProcess, pstAi);
#else
	THR_EntityStart(videoInputProcess, videoInputCleanup, pstAi, &pstAi->stAiPid);
#endif	

EXIT:
    return 0;
}


int SAMPLE_PROC_VIDEO_DestoryTrdAi(int _s32Chnnl)
{
    T_SampleAiInfo	*pstAi = NULL;
    pstAi = &g_tSampleAi[_s32Chnnl - 1];

    if (pstAi->bStart)
    {
		//执行每个node的取消点
		pstAi->bStart = HI_FALSE;
		//if (pstAi->pAiCancel)
		{
			//pstAi->pAiCancel(pstAi->pCancelArg);
		}
        pthread_join(pstAi->stAiPid, 0);
    }

    return 0;
}

int SAMPLE_PROC_VIDEO_SetCancel(int _s32Chnnl, HI_VIDEO_CANCEL _pCancel, void *_pArg)
{

	
	return 0;
}



