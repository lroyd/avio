/*************************************************************************
	> File Name: 
	> Author: lroyd
	> Mail: htzhangxmu@163.com
	> Created Time: 
 ************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/prctl.h>
#include <sys/un.h>
#include <sys/epoll.h>
#include <sys/shm.h>

#include "config_site.h"
#include "avio_api.h"
#include "sample_comm.h"

#include "config_vi_chnnl.h"


////////////////////////////////////////////////////
//只有一路音频
T_AudioConfigInfo g_tAudioConfigInfo = {0};



static int s32AiChnCnt = 1;
static int s32AoChnCnt = 1;
static int	AiDev = 0;
static int	AiChn = 0;
static int	AoDev = 0;
static int	AoChn = 0;
////////////////////////////////////////////////////
const T_VideoChnnlInfo *g_tVideoChnnlTable[HI_VIDEO_CHNNL_NUM] = {NULL};


VIDEO_NORM_E gs_enNorm = VIDEO_ENCODING_MODE_NTSC;
SAMPLE_RC_E enRcMode= SAMPLE_RC_VBR;
////////////////////////////////////////////////////
T_VideoConfigInfo g_tVideoConfigInfo = {0};

///////////////////////////////////////////////////
static FILE *pSaveFile[HI_VIDEO_CHNNL_NUM] = {NULL};
static const char *pSaveFileName[HI_VIDEO_CHNNL_NUM] = {
	"stream_chnn1.h264",
	"stream_chnn2.h264",
	"stream_chnn3.h264"
};


static int audioInit(void)
{
	int s32Ret = -1;
    AIO_ATTR_S stAioAttr;
	AUDIO_SAMPLE_RATE_E enInSampleRate;
	AUDIO_SAMPLE_RATE_E enOutSampleRate;
	
    stAioAttr.enSamplerate   = HI_AUDIO_SAMPLE_RATE;
    stAioAttr.enBitwidth     = HI_AUDIO_BIT_WIDTH;
    stAioAttr.enWorkmode     = AIO_MODE_I2S_MASTER;
    stAioAttr.enSoundmode    = AUDIO_SOUND_MODE_MONO;
    stAioAttr.u32EXFlag      = 0;
    stAioAttr.u32FrmNum      = 30;
    stAioAttr.u32PtNumPerFrm = HI_AUDIO_PTNUMPERFRM;
    stAioAttr.u32ChnCnt      = 1;
    stAioAttr.u32ClkSel      = 0;

	enInSampleRate  = AUDIO_SAMPLE_RATE_BUTT;
	enOutSampleRate = AUDIO_SAMPLE_RATE_BUTT;	
	
    s32Ret = SAMPLE_COMM_AUDIO_CfgAcodec(&stAioAttr);
    if (HI_SUCCESS != s32Ret)
    {
        printf("audio config acodec error\n");
        return -1;
    }

	AI_VQE_CONFIG_S stAiVqeAttr;	
	AO_VQE_CONFIG_S stAoVqeAttr;
	HI_VOID     *pAiVqeAttr = NULL, *pAoVqeAttr = NULL;
	HI_U32		u32AiVqeType = 0, u32AoVqeType = 0;
	
	if (g_tAudioConfigInfo.m_bEC)
	{
		stAiVqeAttr.s32WorkSampleRate    = HI_AUDIO_SAMPLE_RATE;
		stAiVqeAttr.s32FrameSample       = HI_AUDIO_PTNUMPERFRM;
		stAiVqeAttr.enWorkstate          = VQE_WORKSTATE_COMMON;
		stAiVqeAttr.bAecOpen             = HI_TRUE;
		stAiVqeAttr.stAecCfg.bUsrMode    = HI_FALSE;
		stAiVqeAttr.stAecCfg.s8CngMode   = 0;
		stAiVqeAttr.bAgcOpen             = HI_TRUE;
		stAiVqeAttr.stAgcCfg.bUsrMode    = HI_FALSE;
		stAiVqeAttr.bAnrOpen             = HI_TRUE;
		stAiVqeAttr.stAnrCfg.bUsrMode    = HI_FALSE;
		stAiVqeAttr.bHpfOpen             = HI_TRUE;
		stAiVqeAttr.stHpfCfg.bUsrMode    = HI_TRUE;
		stAiVqeAttr.stHpfCfg.enHpfFreq   = AUDIO_HPF_FREQ_150;
		stAiVqeAttr.bRnrOpen             = HI_FALSE;
		stAiVqeAttr.bEqOpen              = HI_FALSE;
		stAiVqeAttr.bHdrOpen             = HI_FALSE;
		pAiVqeAttr = (HI_VOID *)&stAiVqeAttr;

		memset(&stAoVqeAttr, 0, sizeof(AO_VQE_CONFIG_S));
		stAoVqeAttr.s32WorkSampleRate    = HI_AUDIO_SAMPLE_RATE;
		stAoVqeAttr.s32FrameSample       = HI_AUDIO_PTNUMPERFRM;
		stAoVqeAttr.enWorkstate          = VQE_WORKSTATE_COMMON;
		stAoVqeAttr.stAgcCfg.bUsrMode    = HI_FALSE;
		stAoVqeAttr.stAnrCfg.bUsrMode    = HI_FALSE;
		stAoVqeAttr.stHpfCfg.bUsrMode    = HI_TRUE;
		stAoVqeAttr.stHpfCfg.enHpfFreq   = AUDIO_HPF_FREQ_150;

		stAoVqeAttr.bAgcOpen = HI_TRUE;
		stAoVqeAttr.bAnrOpen = HI_TRUE;
		stAoVqeAttr.bEqOpen  = HI_TRUE;
		stAoVqeAttr.bHpfOpen = HI_TRUE;
		
		pAoVqeAttr = (HI_VOID *)&stAoVqeAttr;
		u32AiVqeType = 1;
		u32AoVqeType = 1;		
	}

    s32Ret = SAMPLE_PROC_AUDIO_StartAi(AiDev, s32AiChnCnt, &stAioAttr, enOutSampleRate, HI_FALSE, pAiVqeAttr, u32AiVqeType, g_tAudioConfigInfo.m_s32AiVolume);
    if (s32Ret)
    {
		printf("audio start ai error\n");
        return -1;
    }
	
    s32Ret = SAMPLE_COMM_AUDIO_StartAo(AoDev, s32AoChnCnt, &stAioAttr, enInSampleRate, HI_FALSE, pAoVqeAttr, u32AoVqeType);
    if (s32Ret)
    {
		printf("audio start ao error\n");
        return -1;
    }
    
	SAMPLE_COMM_AUDIO_SetAoVolume(AoDev, g_tAudioConfigInfo.m_s32AoVolume);

    printf("ai(%d,%d) bind to ao(%d,%d) ok\n", AiDev, AiChn, AoDev, AoChn);
	
	return s32Ret;
}

static int audioDeinit(void)
{
	int s32Ret = 0;
	
	if (g_tAudioConfigInfo.m_bLookup)
	{
		SAMPLE_COMM_AUDIO_DestoryTrdAi(AiDev, AiChn);
	}
	else
	{
		SAMPLE_PROC_AUDIO_DestoryTrdAi(AiDev, AiChn);
		SAMPLE_PROC_AUDIO_DestoryTrdAo(AoDev, AoChn);		
	}

#if 0
    s32Ret = SAMPLE_COMM_AUDIO_StopAi(AiDev, s32AiChnCnt, HI_FALSE, HI_FALSE);
    if (s32Ret)
    {
        printf("audio stop ai error\n");
        return -1;
    }
    
    s32Ret = SAMPLE_COMM_AUDIO_StopAo(AoDev, s32AoChnCnt, HI_FALSE, HI_FALSE);
    if (s32Ret)
    {
        printf("audio stop ao error\n");
    }
#endif
	return s32Ret;	
}


int HI_AVIO_AudioSStart(HI_AUDIO_CBK _pAudioCap, HI_AUDIO_CBK _pAudioPlay)
{
	int s32Ret = -1;
	if (g_tAudioConfigInfo.m_bEnable)
	{
		if (g_tAudioConfigInfo.m_bLookup)
		{
			s32Ret = SAMPLE_COMM_AUDIO_CreatTrdAiAo(AiDev, AiChn, AoDev, AoChn);
			if (s32Ret)
			{
				printf("audio start thread ai-ao error\n");
				goto EXIT;
			}			
		}
		else
		{
			
			if (_pAudioCap)
			{
				s32Ret = SAMPLE_PROC_AUDIO_CreatTrdAi(AiDev, AiChn, _pAudioCap, g_tAudioConfigInfo.m_bAiSave);
				if (s32Ret)
				{
					printf("audio start thread ai error\n");
					goto EXIT;
				}
			}

			if (_pAudioPlay)
			{
				s32Ret = SAMPLE_PROC_AUDIO_CreatTrdAo(AoDev, AoChn, _pAudioPlay, g_tAudioConfigInfo.m_bAoSave);
				if (s32Ret)
				{
					printf("audio start thread ao error\n");
					goto EXIT;
				}		
			}			
			
		}		
	}
	else
	{
		printf("audio is not supported!\n");
	}
	
EXIT:	
	return s32Ret;
}

int HI_AVIO_AudioSStop(void)
{
	if (g_tAudioConfigInfo.m_bEnable)
	{
		if (g_tAudioConfigInfo.m_bLookup)
		{
			SAMPLE_COMM_AUDIO_DestoryTrdAi(AiDev, AiChn);
		}
		else
		{
			SAMPLE_PROC_AUDIO_DestoryTrdAi(AiDev, AiChn);
			SAMPLE_PROC_AUDIO_DestoryTrdAo(AoDev, AoChn);			
		}		
	}
	else
	{
		printf("audio is not supported!\n");
	}

	return 0;
}

int HI_AVIO_AudioSetPlayVolume(char _s8Volume)
{
	if (g_tAudioConfigInfo.m_bEnable)
	{
		SAMPLE_COMM_AUDIO_SetAoVolume(AoDev, _s8Volume);
	}
	else
	{
		printf("audio is not supported!\n");
	}		
}

int HI_AVIO_AudioPlayImmt(char *_pData, int _s32Len)
{
	if (g_tAudioConfigInfo.m_bEnable)
	{
		//if (_s32Len == (HI_AUDIO_PTNUMPERFRM * (HI_AUDIO_BIT_WIDTH + 1)))
		{
			SAMPLE_PROC_AUDIO_Play(_pData, _s32Len);
		}		
	}
	else
	{
		printf("audio is not supported!\n");
	}
			
	return 0;
}


static int videoInit(void)
{
	int s32Ret = -1, i;
	int s32ChnNum = HI_VIDEO_CHNNL_NUM;
	SIZE_S stSize;
    VPSS_GRP VpssGrp = 0;
    VPSS_CHN VpssChn = 0;
	VENC_CHN VencChn = 0;
    VPSS_GRP_ATTR_S stVpssGrpAttr;
    VPSS_CHN_ATTR_S stVpssChnAttr;
    VPSS_CHN_MODE_S stVpssChnMode;
	
	SAMPLE_VI_CONFIG_S stViConfig = {0};
	
    stViConfig.enViMode   = SENSOR_TYPE;
    stViConfig.enRotate   = ROTATE_NONE;
    stViConfig.enNorm     = VIDEO_ENCODING_MODE_AUTO;
    stViConfig.enViChnSet = VI_CHN_SET_NORMAL;
    stViConfig.enWDRMode  = WDR_MODE_NONE;

    s32Ret = SAMPLE_COMM_VI_StartVi(&stViConfig);
    if (s32Ret)
    {
        printf("start vi failed!\n");
        goto EXIT_1;
    }	

	s32Ret = SAMPLE_COMM_SYS_GetPicSize(gs_enNorm, g_tVideoConfigInfo.m_s32GroupSize, &stSize);
	if (HI_SUCCESS != s32Ret)
	{
		printf("SAMPLE_COMM_SYS_GetPicSize failed!\n");
		goto EXIT_1;
	}
	
	stVpssGrpAttr.u32MaxW = stSize.u32Width;
	stVpssGrpAttr.u32MaxH = stSize.u32Height;
	stVpssGrpAttr.bIeEn = HI_FALSE;
	stVpssGrpAttr.bNrEn = HI_TRUE;
	stVpssGrpAttr.bHistEn = HI_FALSE;
	stVpssGrpAttr.bDciEn = HI_FALSE;
	stVpssGrpAttr.enDieMode = VPSS_DIE_MODE_NODIE;
	stVpssGrpAttr.enPixFmt = PIXEL_FORMAT_YUV_SEMIPLANAR_420;
	
	s32Ret = SAMPLE_PROC_VPSS_StartGroup(VpssGrp, &stVpssGrpAttr, &g_tVideoConfigInfo.in_tCrop);
	if (s32Ret)
	{
		printf("Start Vpss failed!\n");
		goto EXIT_2;
	}

	s32Ret = SAMPLE_COMM_VI_BindVpss(stViConfig.enViMode);
	if (s32Ret)
	{
		printf("Vi bind Vpss failed!\n");
		goto EXIT_3;
	}

	memset(&stVpssChnAttr, 0, sizeof(stVpssChnAttr));

	//printf("=== table addr[0x%x.8], 1[0x%x.8] 2[0x%x.8] 3[0x%x.8]\n", g_tVideoChnnlTable, g_tVideoChnnlTable[0], g_tVideoChnnlTable[1], g_tVideoChnnlTable[2]);
	
	for(i = 0; i < s32ChnNum; i++)
	{
		if (g_tVideoChnnlTable[i])
		{
			
			s32Ret = SAMPLE_COMM_SYS_GetPicSize(gs_enNorm, g_tVideoChnnlTable[i]->m_u8PixSize, &stSize);
			if (s32Ret)
			{
				printf("SAMPLE_COMM_SYS_GetPicSize failed!\n");
				goto EXIT_4;
			}
			
			VpssChn = g_tVideoChnnlTable[i]->m_u8Chnnl - 1;	//-1校准
			VencChn = g_tVideoChnnlTable[i]->m_u8Chnnl - 1;
			stVpssChnMode.enChnMode 	= VPSS_CHN_MODE_USER;
			stVpssChnMode.bDouble		= HI_FALSE;
			stVpssChnMode.enPixelFormat = PIXEL_FORMAT_YUV_SEMIPLANAR_420;
			stVpssChnMode.u32Width		= stSize.u32Width;
			stVpssChnMode.u32Height 	= stSize.u32Height;
			stVpssChnMode.enCompressMode = g_tVideoChnnlTable[i]->m_u8Compress;
			stVpssChnAttr.s32SrcFrameRate = -1;
			stVpssChnAttr.s32DstFrameRate = -1;	
			s32Ret = SAMPLE_COMM_VPSS_EnableChn(VpssGrp, VpssChn, &stVpssChnAttr, &stVpssChnMode, HI_NULL);
			if (s32Ret)
			{
				printf("Enable vpss chn failed!\n");
				goto EXIT_4;
			}
			
			s32Ret = SAMPLE_COMM_VENC_Start(VencChn, g_tVideoChnnlTable[i]->m_u8PayLoad, \
											gs_enNorm, g_tVideoChnnlTable[i]->m_u8PixSize, \
											enRcMode, g_tVideoChnnlTable[i]->m_u8Profile);
			if (s32Ret)
			{
				printf("Start Venc failed!\n");
				goto EXIT_5;
			}

			s32Ret = SAMPLE_COMM_VENC_BindVpss(VencChn, VpssGrp, VpssChn);
			if (s32Ret)
			{
				printf("Start Venc failed!\n");
				goto EXIT_5;
			}
		}
		
	}

	printf("video init ok\n");
	return 0;
	
	
EXIT_5:
	
	for(i = 0; i < s32ChnNum; i++)
	{
		if (g_tVideoChnnlTable[i])
		{
			VpssChn = g_tVideoChnnlTable[i]->m_u8Chnnl - 1;	//-1校准
			VencChn = g_tVideoChnnlTable[i]->m_u8Chnnl - 1;			
			SAMPLE_COMM_VENC_UnBindVpss(VencChn, VpssGrp, VpssChn);
			SAMPLE_COMM_VENC_Stop(VencChn);			
			
		}
	}

    SAMPLE_COMM_VI_UnBindVpss(stViConfig.enViMode);
	
EXIT_4:	
	for(i = 0; i < s32ChnNum; i++)
	{
		if (g_tVideoChnnlTable[i])
		{
			VpssChn = g_tVideoChnnlTable[i]->m_u8Chnnl - 1;
			SAMPLE_COMM_VPSS_DisableChn(VpssGrp, VpssChn);	
		}
	}
	
EXIT_3:     
    SAMPLE_COMM_VI_UnBindVpss(stViConfig.enViMode);
EXIT_2:    
    SAMPLE_COMM_VPSS_StopGroup(VpssGrp);
EXIT_1:	
    SAMPLE_COMM_VI_StopVi(&stViConfig);

	return s32Ret;
}


static int videoDeinit(void)
{
	int i;
	//printf("===--- table addr[0x%x.8], 1[0x%x.8] 2[0x%x.8] 3[0x%x.8]\n", g_tVideoChnnlTable, g_tVideoChnnlTable[0], g_tVideoChnnlTable[1], g_tVideoChnnlTable[2]);
	for(i = 0; i < HI_VIDEO_CHNNL_NUM; i++)
	{
		if (g_tVideoChnnlTable[i])
		{
			HI_AVIO_VideoStopChannel(i+1);
		}
	}
	
	return 0;
}


int HI_AVIO_VideoRegisterServer(int _s32Chnnl, char *_pName, HI_VIDEO_CBK _pHandle, HI_VIDEO_CANCEL _pCancel, void *_pArg)
{
	int s32Ret = -1;
	if (g_tVideoConfigInfo.m_bEnable && _s32Chnnl > 0)
	{
		if (g_tVideoChnnlTable[_s32Chnnl - 1])
		{		
			if (_pHandle)
			{
				s32Ret = SAMPLE_PROC_VIDEO_RegisterServer(_s32Chnnl, _pName, _pHandle, _pCancel, _pArg);
			}
			else
			{
				printf("video channel [%d] server can not empty!!!\n", _s32Chnnl);
			}
		}
		else
		{
			printf("video channel [%d] is not open!!!\n", _s32Chnnl);
		}
	}
	else
	{
		printf("video is not supported!\n");
	}	
}


int HI_AVIO_VideoStartChannel(int _s32Chnnl)
{
	int s32Ret = -1;
	
	if (g_tVideoConfigInfo.m_bEnable && _s32Chnnl > 0)
	{
		if (g_tVideoChnnlTable[_s32Chnnl - 1])
		{	
			s32Ret = SAMPLE_PROC_VIDEO_CreatTrdAi(_s32Chnnl);
		}
		else
		{
			printf("video channel [%d] is not open!!!\n", _s32Chnnl);
		}		
	}
	else
	{
		printf("video is not supported!\n");
	}
	
	return s32Ret;
}


int HI_AVIO_VideoStopChannel(int _s32Chnnl)
{
	int s32Ret = -1;
	if (g_tVideoConfigInfo.m_bEnable && _s32Chnnl > 0)
	{
		if (g_tVideoChnnlTable[_s32Chnnl - 1])
		{
			s32Ret = SAMPLE_PROC_VIDEO_DestoryTrdAi(_s32Chnnl);
		}
		else
		{
			printf("video channel [%d] is not open!!!\n", _s32Chnnl);
		}		
	}
	else
	{
		printf("video is not supported!\n");
	}

	return s32Ret;
}


////////////////////////////////////////////////////
//解析参数：音频+视频
static int defaultParseConfig(void)
{
	/* 音频参数 */
	T_AudioConfigInfo *pAudio = &g_tAudioConfigInfo;
#ifdef HI_AVIO_AUDIO_ON
	pAudio->m_bEnable = 1;
#endif	
	
	if (pAudio->m_bEnable)
	{
#if 0		
#ifdef HI_AUDIO_SAMPLE_RATE
		pAudio->in_tAudio.m_u32Sample = HI_AUDIO_SAMPLE_RATE;
#endif

#ifdef 	HI_AUDIO_PTNUMPERFRM
		pAudio->in_tAudio.m_u32PtNumPerfrm = HI_AUDIO_PTNUMPERFRM;
#endif

#ifdef HI_AUDIO_BIT_WIDTH
		pAudio->in_tAudio.m_u32BitWidth = HI_AUDIO_BIT_WIDTH;
#endif
#endif		
		
#ifdef HI_AUDIO_EC_ON	
		pAudio->m_bEC = 1;
#endif

#ifdef HI_AUDIO_LOOKUP_ON
		pAudio->m_bLookup = 1;
#endif

#ifdef HI_AUDIO_AI_VOLUME
		pAudio->m_s32AiVolume = HI_AUDIO_AI_VOLUME;
#endif

#ifdef HI_AUDIO_AO_VOLUME
		pAudio->m_s32AoVolume = HI_AUDIO_AO_VOLUME;
#endif

#ifdef HI_AUDIO_CAP_SAVE_FILE_ON
		pAudio->m_bAiSave = 1;
#endif

#ifdef HI_AUDIO_PLAY_SAVE_FILE_ON
		pAudio->m_bAoSave = 1;
#endif

	}
	
	/* 视频参数 */
	T_VideoConfigInfo *pVideo = &g_tVideoConfigInfo;
	
#ifdef HI_AVIO_VIDEO_ON
	pVideo->m_bEnable = 1;
#endif	

	if (pVideo->m_bEnable)
	{
		pVideo->m_s32GroupSize = HI_VIDEO_GROUP_SIZE; 
		
#ifdef HI_VIDEO_VENC_SAVE_FILE_ON
		pVideo->m_bSave = 1;
#endif		
		
#ifdef HI_VIDEO_GROUP_CROP	
		pVideo->in_tCrop.m_bEnable = 1;
		pVideo->in_tCrop.m_u32X	= HI_VIDEO_CROP_RECT_X;
		pVideo->in_tCrop.m_u32Y	= HI_VIDEO_CROP_RECT_Y;
		pVideo->in_tCrop.m_u32W	= HI_VIDEO_CROP_RECT_W;
		pVideo->in_tCrop.m_u32H	= HI_VIDEO_CROP_RECT_H;
		
#endif	


#ifdef HI_VI_CHNNL_1_ON
		g_tVideoChnnlTable[0] = &ctUseChnnl_1;
#endif

#ifdef HI_VI_CHNNL_2_ON
		g_tVideoChnnlTable[1] = &ctUseChnnl_2;
#endif

#ifdef HI_VI_CHNNL_3_ON
		g_tVideoChnnlTable[2] = &ctUseChnnl_3;
#endif		
		
	}
	
	return 0;
}



////////////////////////////////////////////////////
static int videoSaveFile(int __s32Chnnl, int _u8NalType, char *_pData, int _u32Len, unsigned long long _u64Ptime)
{
	//printf("chn %d, nal type %d, len %d\r\n", __s32Chnnl, _u8NalType, _u32Len);
	
	fwrite(_pData, _u32Len, 1, pSaveFile[__s32Chnnl - 1]);
	fflush(pSaveFile[__s32Chnnl - 1]);		
	return 0;
}


int HI_AVIO_Init(const char *_pConfigPath)
{
	int s32Ret = -1, i, j, k = 0;
	unsigned int u32BlkSize, u32BlkCnt = 4;
	VB_CONF_S stVbConf;

	//先加载配置文件
	if (_pConfigPath)
	{
		iniAvioParseConfig(_pConfigPath, &g_tAudioConfigInfo, &g_tVideoConfigInfo, g_tVideoChnnlTable);
	}
	else
	{
		defaultParseConfig();
	}	
	
	
	memset(&stVbConf,0,sizeof(VB_CONF_S));

	if (g_tVideoConfigInfo.m_bEnable)
	{
		for (j = 0; j < HI_VIDEO_CHNNL_NUM; j++)
		{
			for(i = k; i < HI_VIDEO_CHNNL_NUM; i++)
			{
				if (g_tVideoChnnlTable[i])
				{
					//720P	5-6M	1080P 11-12M	480P  2M	1M
					u32BlkSize = SAMPLE_COMM_SYS_CalcPicVbBlkSize(gs_enNorm, g_tVideoChnnlTable[i]->m_u8PixSize, \
																	SAMPLE_PIXEL_FORMAT, SAMPLE_SYS_ALIGN_WIDTH);
					stVbConf.astCommPool[j].u32BlkSize	= u32BlkSize;
					stVbConf.astCommPool[j].u32BlkCnt	= g_tVideoChnnlTable[i]->m_u8BlkCnt;
					printf("use channel [%d] pool size = %d, cnt = %d\n", g_tVideoChnnlTable[i]->m_u8Chnnl, u32BlkSize, g_tVideoChnnlTable[i]->m_u8BlkCnt);	
					
					k = i + 1;
					break;
				}
			}		
		}
		if (k == 0)
		{
			//没有可用通道配置，err函数直接返回
			printf("No channel is available. Configure the channel first\n");
			//goto EXIT;
		}
		
		stVbConf.u32MaxPoolCnt = 128;
	}

	SAMPLE_PROC_VIDEO_Init();

    s32Ret = SAMPLE_COMM_SYS_Init(&stVbConf);
    if (s32Ret)
    {
        printf("%s: system init failed with %d!\n", __FUNCTION__, s32Ret);
        goto EXIT;
    }

	if (g_tAudioConfigInfo.m_bEnable)
	{
		s32Ret = audioInit();
		if (s32Ret)
		{
			printf("%s: audio init failed\n", __FUNCTION__);
			goto EXIT;
		}		
	}
	
	if (g_tVideoConfigInfo.m_bEnable)
	{
		s32Ret = videoInit();
		if (s32Ret)
		{
			printf("%s: video init failed\n", __FUNCTION__);
			goto EXIT;
		}		
	}

	if (g_tVideoConfigInfo.m_bSave)
	{
		printf("user save file mode ON:\n");
		for(i = 0; i < HI_VIDEO_CHNNL_NUM; i++)
		{
			pSaveFile[i] = fopen(pSaveFileName[i], "wb");
			printf("opens file = %s\n", pSaveFileName[i]);
			HI_AVIO_VideoRegisterServer(i + 1, "save", videoSaveFile, NULL, NULL);
		}
			
	}
	
	return 0;
	
EXIT:
    SAMPLE_COMM_SYS_Exit();

	return s32Ret;
}

int HI_AVIO_Deinit(void)
{
	if (g_tAudioConfigInfo.m_bEnable)
	{
		audioDeinit();
	}

	if (g_tVideoConfigInfo.m_bEnable)
	{
		videoDeinit();
		
		if (g_tVideoConfigInfo.m_bSave)
		{
			int i;
			for(i = 0; i < HI_VIDEO_CHNNL_NUM; i++)
			{
				fclose(pSaveFile[i]);
				printf("user close all save file\n");
			}
				
		}		
		SAMPLE_COMM_ISP_Stop();	
	}
	SAMPLE_COMM_SYS_Exit();	

	return 0;
}

void HI_AVIO_SignalHandle(int _s32No)
{
    if (SIGINT == _s32No || SIGTERM == _s32No)
    {
		HI_AVIO_Deinit();
        printf("\033[0;31mavio program termination abnormally!\033[0;39m\n");
    }
    exit(-1);
}




