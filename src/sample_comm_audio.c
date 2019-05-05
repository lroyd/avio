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

#define SMMP_AUDIO_CAP_ID	(12345)	
#define SMMP_AUDIO_PB_ID	(67890)	
#define HI_AUDIO_SMM_LOOKUP_ON

//#define HI_ACODEC_TYPE_INNER		//+++


#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* End of #ifdef __cplusplus */

#define ACODEC_FILE     "/dev/acodec"

#define AUDIO_ADPCM_TYPE ADPCM_TYPE_DVI4/* ADPCM_TYPE_IMA, ADPCM_TYPE_DVI4*/
#define G726_BPS MEDIA_G726_40K         /* MEDIA_G726_16K, MEDIA_G726_24K ... */


typedef struct tagSAMPLE_AENC_S
{
    HI_BOOL bStart;
    pthread_t stAencPid;
	pthread_t stAencPidPlay;
    HI_S32  AeChn;
    HI_S32  AdChn;
    FILE    *pfd;
    HI_BOOL bSendAdChn;
} SAMPLE_AENC_S;

typedef struct tagSAMPLE_AI_S
{
    HI_BOOL bStart;
    HI_S32  AiDev;
    HI_S32  AiChn;
    HI_S32  AencChn;
    HI_S32  AoDev;
    HI_S32  AoChn;
    HI_BOOL bSendAenc;
    HI_BOOL bSendAo;
    pthread_t stAiPid;
} SAMPLE_AI_S;

typedef struct tagSAMPLE_ADEC_S
{
    HI_BOOL bStart;
    HI_S32 AdChn; 
    FILE *pfd;
    pthread_t stAdPid;
} SAMPLE_ADEC_S;

typedef struct tagSAMPLE_AO_S
{
	AUDIO_DEV AoDev;
	HI_BOOL bStart;
	HI_S32  AoChn;
	pthread_t stAoPid;
}SAMPLE_AO_S;

static SAMPLE_AI_S gs_stSampleAi[2];//[AI_DEV_MAX_NUM*AIO_MAX_CHN_NUM];
static SAMPLE_AENC_S gs_stSampleAenc[2];//[AENC_MAX_CHN_NUM];
static SAMPLE_ADEC_S gs_stSampleAdec[2];//[ADEC_MAX_CHN_NUM];
static SAMPLE_AO_S   gs_stSampleAo[2];//[AO_DEV_MAX_NUM];

#define CONTEXT_SZ (360)
struct shared_use_st  
{  
    int written;//作为一个标志，非0：表示可读，0表示可写  
    int packet_type;
	int packet_len;
	unsigned char video_road;
    unsigned char text[CONTEXT_SZ];//记录写入和读取的文本  
};

HI_S32 SAMPLE_INNER_CODEC_CfgAudio(AUDIO_SAMPLE_RATE_E enSample)
{
    HI_S32 fdAcodec = -1;
    HI_S32 ret = HI_SUCCESS;
    ACODEC_FS_E i2s_fs_sel = 0;
    int iAcodecInputVol = 0;
    ACODEC_MIXER_E input_mode = 0;

    fdAcodec = open(ACODEC_FILE, O_RDWR);
    if (fdAcodec < 0)
    {
        printf("%s: can't open Acodec,%s\n", __FUNCTION__, ACODEC_FILE);
        return HI_FAILURE;
    }
    if(ioctl(fdAcodec, ACODEC_SOFT_RESET_CTRL))
    {
    	printf("Reset audio codec error\n");
    }

	switch (enSample)
	{
		case AUDIO_SAMPLE_RATE_8000:
			i2s_fs_sel = ACODEC_FS_8000;
			break;

		case AUDIO_SAMPLE_RATE_16000:
			i2s_fs_sel = ACODEC_FS_16000;
			break;

		case AUDIO_SAMPLE_RATE_32000:
			i2s_fs_sel = ACODEC_FS_32000;
			break;

		case AUDIO_SAMPLE_RATE_11025:
			i2s_fs_sel = ACODEC_FS_11025;
			break;

		case AUDIO_SAMPLE_RATE_22050:
			i2s_fs_sel = ACODEC_FS_22050;
			break;

		case AUDIO_SAMPLE_RATE_44100:
			i2s_fs_sel = ACODEC_FS_44100;
			break;

		case AUDIO_SAMPLE_RATE_12000:
			i2s_fs_sel = ACODEC_FS_12000;
			break;

		case AUDIO_SAMPLE_RATE_24000:
			i2s_fs_sel = ACODEC_FS_24000;
			break;

		case AUDIO_SAMPLE_RATE_48000:
			i2s_fs_sel = ACODEC_FS_48000;
			break;

		case AUDIO_SAMPLE_RATE_64000:
			i2s_fs_sel = ACODEC_FS_64000;
			break;

		case AUDIO_SAMPLE_RATE_96000:
			i2s_fs_sel = ACODEC_FS_96000;
			break;
	
		default:
			printf("%s: not support enSample:%d\n", __FUNCTION__, enSample);
        	ret = HI_FAILURE;
			break;
	}    

    if (ioctl(fdAcodec, ACODEC_SET_I2S1_FS, &i2s_fs_sel)) 
    {
        printf("%s: set acodec sample rate failed\n", __FUNCTION__);
        ret = HI_FAILURE;
    }

    //select IN or IN_Difference
    input_mode = ACODEC_MIXER_IN;
    if (ioctl(fdAcodec, ACODEC_SET_MIXER_MIC, &input_mode)) 
    {
        printf("%s: select acodec input_mode failed\n", __FUNCTION__);
        ret = HI_FAILURE;
    }
    
    
    if (0) /* should be 1 when micin */
    {
        /******************************************************************************************
        The input volume range is [-87, +86]. Both the analog gain and digital gain are adjusted.
        A larger value indicates higher volume.
        For example, the value 86 indicates the maximum volume of 86 dB,
        and the value -87 indicates the minimum volume (muted status).
        The volume adjustment takes effect simultaneously in the audio-left and audio-right channels.
        The recommended volume range is [+10, +56].
        Within this range, the noises are lowest because only the analog gain is adjusted,
        and the voice quality can be guaranteed.
        *******************************************************************************************/
        iAcodecInputVol = 30;
        if (ioctl(fdAcodec, ACODEC_SET_INPUT_VOL, &iAcodecInputVol))
        {
            printf("%s: set acodec micin volume failed\n", __FUNCTION__);
            return HI_FAILURE;
        }

    }
    
    close(fdAcodec);
    return ret;
}


/* config codec */ 
HI_S32 SAMPLE_COMM_AUDIO_CfgAcodec(AIO_ATTR_S *pstAioAttr)
{
    HI_S32 s32Ret = HI_SUCCESS;
    HI_BOOL bCodecCfg = HI_FALSE;

#ifdef HI_ACODEC_TYPE_INNER
    /*** INNER AUDIO CODEC ***/
    s32Ret = SAMPLE_INNER_CODEC_CfgAudio(pstAioAttr->enSamplerate); 
    if (HI_SUCCESS != s32Ret)
    {
        printf("%s:SAMPLE_INNER_CODEC_CfgAudio failed\n", __FUNCTION__);
        return s32Ret;
    }

    bCodecCfg = HI_TRUE;
#endif
    
#ifdef HI_ACODEC_TYPE_TLV320AIC31
    /*** ACODEC_TYPE_TLV320 ***/ 
    s32Ret = SAMPLE_Tlv320_CfgAudio(pstAioAttr->enWorkmode, pstAioAttr->enSamplerate);
    if (HI_SUCCESS != s32Ret)
    {
        printf("%s: SAMPLE_Tlv320_CfgAudio failed\n", __FUNCTION__);
        return s32Ret;
    }
    bCodecCfg = HI_TRUE;
#endif    

    if (!bCodecCfg)
    {
        printf("Can not find the right codec.\n");
        return HI_FALSE;
    }
    return HI_SUCCESS;
}

/******************************************************************************
* function : get frame from Ai, send it  to Aenc or Ao
******************************************************************************/
void *SAMPLE_COMM_AUDIO_AiProc(void *parg)
{
    HI_S32 s32Ret;
    HI_S32 AiFd;
    SAMPLE_AI_S *pstAiCtl = (SAMPLE_AI_S *)parg;
    AUDIO_FRAME_S stFrame; 
	AEC_FRAME_S   stAecFrm;
    fd_set read_fds;
    struct timeval TimeoutVal;
    AI_CHN_PARAM_S stAiChnPara;

    s32Ret = HI_MPI_AI_GetChnParam(pstAiCtl->AiDev, pstAiCtl->AiChn, &stAiChnPara);
    if (HI_SUCCESS != s32Ret)
    {
        printf("%s: Get ai chn param failed\n", __FUNCTION__);
        return NULL;
    }
    //static FILE    *pfd;
	//pfd = fopen("aec_test.pcm", "w+");

    stAiChnPara.u32UsrFrmDepth = 30;
    
    s32Ret = HI_MPI_AI_SetChnParam(pstAiCtl->AiDev, pstAiCtl->AiChn, &stAiChnPara);
    if (HI_SUCCESS != s32Ret)
    {
        printf("%s: set ai chn param failed\n", __FUNCTION__);
        return NULL;
    }
    
    FD_ZERO(&read_fds);
    AiFd = HI_MPI_AI_GetFd(pstAiCtl->AiDev, pstAiCtl->AiChn);
    FD_SET(AiFd,&read_fds);

    while (pstAiCtl->bStart)
    {     
        TimeoutVal.tv_sec = 1;
        TimeoutVal.tv_usec = 0;
        
        FD_ZERO(&read_fds);
        FD_SET(AiFd,&read_fds);
        
        s32Ret = select(AiFd+1, &read_fds, NULL, NULL, &TimeoutVal);
        if (s32Ret < 0) 
        {
            break;
        }
        else if (0 == s32Ret) 
        {
            printf("%s: get ai frame select time out\n", __FUNCTION__);
            break;
        }        
        
        if (FD_ISSET(AiFd, &read_fds))
        {
            /* get frame from ai chn */
			memset(&stAecFrm, 0, sizeof(AEC_FRAME_S));
            s32Ret = HI_MPI_AI_GetFrame(pstAiCtl->AiDev, pstAiCtl->AiChn, &stFrame, &stAecFrm, HI_FALSE);
            if (HI_SUCCESS != s32Ret )
            {
            	#if 0
                printf("%s: HI_MPI_AI_GetFrame(%d, %d), failed with %#x!\n",\
                       __FUNCTION__, pstAiCtl->AiDev, pstAiCtl->AiChn, s32Ret);
                pstAiCtl->bStart = HI_FALSE;
                return NULL;
				#else
				continue;
				#endif
            }
			
			//printf("....... Len = %d, TimeStamp = %d, Seq = %d\r\n", stFrame.u32Len, stFrame.u64TimeStamp, stFrame.u32Seq);
			//fwrite(stFrame.pVirAddr[0],  1 ,stFrame.u32Len , pfd);
            //fflush(pfd);
            /* send frame to encoder */
            if (HI_TRUE == pstAiCtl->bSendAenc)
            {
                s32Ret = HI_MPI_AENC_SendFrame(pstAiCtl->AencChn, &stFrame, &stAecFrm);
                if (HI_SUCCESS != s32Ret )
                {
                    printf("%s: HI_MPI_AENC_SendFrame(%d), failed with %#x!\n",\
                           __FUNCTION__, pstAiCtl->AencChn, s32Ret);
                    pstAiCtl->bStart = HI_FALSE;
                    return NULL;
                }
            }
            
            /* send frame to ao */
            if (HI_TRUE == pstAiCtl->bSendAo)
            {
                s32Ret = HI_MPI_AO_SendFrame(pstAiCtl->AoDev, pstAiCtl->AoChn, &stFrame, 1000);
                if (HI_SUCCESS != s32Ret )
                {
                    printf("%s: HI_MPI_AO_SendFrame(%d, %d), failed with %#x!\n",\
                           __FUNCTION__, pstAiCtl->AoDev, pstAiCtl->AoChn, s32Ret);
                    pstAiCtl->bStart = HI_FALSE;
                    return NULL;
                }
                
            }

            /* finally you must release the stream */
            s32Ret = HI_MPI_AI_ReleaseFrame(pstAiCtl->AiDev, pstAiCtl->AiChn, &stFrame, &stAecFrm);
            if (HI_SUCCESS != s32Ret )
            {
                printf("%s: HI_MPI_AI_ReleaseFrame(%d, %d), failed with %#x!\n",\
                       __FUNCTION__, pstAiCtl->AiDev, pstAiCtl->AiChn, s32Ret);
                pstAiCtl->bStart = HI_FALSE;
                return NULL;
            }
            
        }
    }
    //fclose(pfd);
	
    pstAiCtl->bStart = HI_FALSE;
    return NULL;
}

/******************************************************************************
* function : get stream from Aenc, send it  to Adec & save it to file
******************************************************************************/
void *SAMPLE_COMM_AUDIO_AencProc(void *parg)
{
    HI_S32 s32Ret;
    HI_S32 AencFd;
    SAMPLE_AENC_S *pstAencCtl = (SAMPLE_AENC_S *)parg;
    AUDIO_STREAM_S stStream;
    fd_set read_fds;
    struct timeval TimeoutVal;
    
    FD_ZERO(&read_fds);    
    AencFd = HI_MPI_AENC_GetFd(pstAencCtl->AeChn);
    FD_SET(AencFd, &read_fds);

	//采集
	struct shared_use_st *shared_cap;//指向shm  
	int shmid_cap;//共享内存标识符	
	void *rshm = NULL;
    //shmid_cap = shmget((key_t)67890, sizeof(struct shared_use_st), 0666|IPC_CREAT); 
	shmid_cap = shmget((key_t)12345, sizeof(struct shared_use_st), 0666|IPC_CREAT); 
    if(shmid_cap == -1)  
    {  
        fprintf(stderr, "shmget failed\n");  
        exit(EXIT_FAILURE);  
    }
    //将共享内存连接到当前进程的地址空间  
    rshm = shmat(shmid_cap, 0, 0);  
    if(rshm == (void*)-1)  
    {  
        fprintf(stderr, "shmat failed\n");  
        exit(EXIT_FAILURE);  
    }  
    printf("\n Cap Memory attached at A33 %X\n", (int)rshm); 	
    shared_cap = (struct shared_use_st*)rshm;  
	shared_cap->written = 0; 	
	
	
	
    while (pstAencCtl->bStart)
    {     
#if 1
        TimeoutVal.tv_sec = 1;
        TimeoutVal.tv_usec = 0;
        
        FD_ZERO(&read_fds);
        FD_SET(AencFd,&read_fds);
        
        s32Ret = select(AencFd+1, &read_fds, NULL, NULL, &TimeoutVal);
        if (s32Ret < 0) 
        {
            break;
        }
        else if (0 == s32Ret) 
        {
            printf("%s: get aenc stream select time out\n", __FUNCTION__);
            break;
        }
        
        if (FD_ISSET(AencFd, &read_fds))
        {
            /* get stream from aenc chn */
            s32Ret = HI_MPI_AENC_GetStream(pstAencCtl->AeChn, &stStream, HI_FALSE);
            if (HI_SUCCESS != s32Ret )
            {
                printf("%s: HI_MPI_AENC_GetStream(%d), failed with %#x!\n",\
                       __FUNCTION__, pstAencCtl->AeChn, s32Ret);
                pstAencCtl->bStart = HI_FALSE;
                return NULL;
            }
   
			/* 1.先采集发送 */
            //printf("enc len = %d, [0x%02x][0x%02x][0x%02x][0x%02x]\r\n",stStream.u32Len,stStream.pStream[0],stStream.pStream[1],stStream.pStream[2],stStream.pStream[3]);          

			//printf("enc len = %d\r\n", stStream.u32Len);
#if 1
			while(shared_cap->written == 1) //0可写，不可读
			{
				usleep(100);
			}

			memset(shared_cap->text, 0, 320);  //海思采集必须是320
			memcpy(shared_cap->text, &stStream.pStream[4], stStream.u32Len - 4);
			shared_cap->packet_len = stStream.u32Len - 4;			

			shared_cap->written = 1;
#endif			
			            /* finally you must release the stream */
            s32Ret = HI_MPI_AENC_ReleaseStream(pstAencCtl->AeChn, &stStream);
            if (HI_SUCCESS != s32Ret )
            {
                printf("%s: HI_MPI_AENC_ReleaseStream(%d), failed with %#x!\n",\
                       __FUNCTION__, pstAencCtl->AeChn, s32Ret);
                pstAencCtl->bStart = HI_FALSE;
                return NULL;
            }
			
        }    
#endif
    }
    
    fclose(pstAencCtl->pfd);
    pstAencCtl->bStart = HI_FALSE;
    return NULL;
}

/******************************************************************************
* function : get stream from file, and send it  to Adec
******************************************************************************/
void *SAMPLE_COMM_AUDIO_AdecProc(void *parg)
{
    HI_S32 s32Ret;
    AUDIO_STREAM_S stAudioStream;    
    HI_U32 u32Len = 640;
    HI_U32 u32ReadLen;
    HI_S32 s32AdecChn;
    HI_U8 *pu8AudioStream = NULL;
    SAMPLE_ADEC_S *pstAdecCtl = (SAMPLE_ADEC_S *)parg;    
    FILE *pfd = pstAdecCtl->pfd;
    s32AdecChn = pstAdecCtl->AdChn;
    
    pu8AudioStream = (HI_U8*)malloc(sizeof(HI_U8)*MAX_AUDIO_STREAM_LEN);
    if (NULL == pu8AudioStream)
    {
        printf("%s: malloc failed!\n", __FUNCTION__);
        return NULL;
    }

    while (HI_TRUE == pstAdecCtl->bStart)
    {
        /* read from file */
        stAudioStream.pStream = pu8AudioStream;
        u32ReadLen = fread(stAudioStream.pStream, 1, u32Len, pfd);
        if (u32ReadLen <= 0)
        {
            s32Ret = HI_MPI_ADEC_SendEndOfStream(s32AdecChn, HI_FALSE);
            if (HI_SUCCESS != s32Ret)
            {
                printf("%s: HI_MPI_ADEC_SendEndOfStream failed!\n", __FUNCTION__);
            }
            fseek(pfd, 0, SEEK_SET);/*read file again*/
            continue;
        }

        /* here only demo adec streaming sending mode, but pack sending mode is commended */
        stAudioStream.u32Len = u32ReadLen;
        s32Ret = HI_MPI_ADEC_SendStream(s32AdecChn, &stAudioStream, HI_TRUE);
        if(HI_SUCCESS != s32Ret)
        {
            printf("%s: HI_MPI_ADEC_SendStream(%d) failed with %#x!\n",\
                   __FUNCTION__, s32AdecChn, s32Ret);
            break;
        }
    }
    
    free(pu8AudioStream);
    pu8AudioStream = NULL;
    fclose(pfd);
    pstAdecCtl->bStart = HI_FALSE;
    return NULL;
}

/******************************************************************************
* function : set ao volume 
******************************************************************************/
void *SAMPLE_COMM_AUDIO_AoVolProc(void *parg)
{
	HI_S32 s32Ret;
	HI_S32 s32Volume;
	AUDIO_DEV AoDev;
	AUDIO_FADE_S stFade;
	SAMPLE_AO_S *pstAoCtl = (SAMPLE_AO_S *)parg;
	AoDev = pstAoCtl->AoDev;
	
	while(pstAoCtl->bStart)
	{	
		for(s32Volume = 0; s32Volume <=6; s32Volume++)
		{
			s32Ret = HI_MPI_AO_SetVolume( AoDev, s32Volume);
			if(HI_SUCCESS != s32Ret)
			{
                printf("%s: HI_MPI_AO_SetVolume(%d), failed with %#x!\n",\
                       __FUNCTION__, AoDev, s32Ret);
			}
			printf("\rset volume %d          ", s32Volume);
			fflush(stdout);
			sleep(2);		
		}
		
		for(s32Volume = 5; s32Volume >=-15; s32Volume--)
		{
			s32Ret = HI_MPI_AO_SetVolume( AoDev, s32Volume);
			if(HI_SUCCESS != s32Ret)
			{
                printf("%s: HI_MPI_AO_SetVolume(%d), failed with %#x!\n",\
                       __FUNCTION__, AoDev, s32Ret);
			}
			printf("\rset volume %d          ", s32Volume);
			fflush(stdout);
			sleep(2);		
		}
		
        for (s32Volume = -14; s32Volume <= 0; s32Volume++)
		{
			s32Ret = HI_MPI_AO_SetVolume( AoDev, s32Volume);
			if(HI_SUCCESS != s32Ret)
			{
                printf("%s: HI_MPI_AO_SetVolume(%d), failed with %#x!\n",\
                       __FUNCTION__, AoDev, s32Ret);
			}
			printf("\rset volume %d          ", s32Volume);
			fflush(stdout);
			sleep(2);		
		}
		
		stFade.bFade         = HI_TRUE;
		stFade.enFadeInRate  = AUDIO_FADE_RATE_128;
		stFade.enFadeOutRate = AUDIO_FADE_RATE_128;
		
		s32Ret = HI_MPI_AO_SetMute(AoDev, HI_TRUE, &stFade);
		if(HI_SUCCESS != s32Ret)
		{
            printf("%s: HI_MPI_AO_SetVolume(%d), failed with %#x!\n",\
                   __FUNCTION__, AoDev, s32Ret);
		}
		printf("\rset Ao mute            ");
		fflush(stdout);
		sleep(2);
		
		s32Ret = HI_MPI_AO_SetMute(AoDev, HI_FALSE, NULL);
		if(HI_SUCCESS != s32Ret)
		{
            printf("%s: HI_MPI_AO_SetVolume(%d), failed with %#x!\n",\
                   __FUNCTION__, AoDev, s32Ret);
		}
		printf("\rset Ao unmute          ");
		fflush(stdout);
		sleep(2);
	}
	return NULL;
}


void *ca_AiProc(void *parg)
{
    HI_S32 s32Ret;
    HI_S32 AiFd;
    SAMPLE_AI_S *pstAiCtl = (SAMPLE_AI_S *)parg;
    AUDIO_FRAME_S stFrame; 
	AEC_FRAME_S   stAecFrm;
    fd_set read_fds;
    struct timeval TimeoutVal;
    AI_CHN_PARAM_S stAiChnPara;

    s32Ret = HI_MPI_AI_GetChnParam(pstAiCtl->AiDev, pstAiCtl->AiChn, &stAiChnPara);
    if (HI_SUCCESS != s32Ret)
    {
        printf("%s: Get ai chn param failed\n", __FUNCTION__);
        return NULL;
    }

    stAiChnPara.u32UsrFrmDepth = 30;
    
    s32Ret = HI_MPI_AI_SetChnParam(pstAiCtl->AiDev, pstAiCtl->AiChn, &stAiChnPara);
    if (HI_SUCCESS != s32Ret)
    {
        printf("%s: set ai chn param failed\n", __FUNCTION__);
        return NULL;
    }
    
    FD_ZERO(&read_fds);
    AiFd = HI_MPI_AI_GetFd(pstAiCtl->AiDev, pstAiCtl->AiChn);
    FD_SET(AiFd,&read_fds);

	//采集
	struct shared_use_st *shared_cap;//指向shm  
	int shmid_cap;//共享内存标识符	
	void *rshm = NULL;
    shmid_cap = shmget((key_t)12345, sizeof(struct shared_use_st), 0666|IPC_CREAT); 
    if(shmid_cap == -1)  
    {  
        fprintf(stderr, "shmget failed\n");  
        exit(EXIT_FAILURE);  
    }
    //将共享内存连接到当前进程的地址空间  
    rshm = shmat(shmid_cap, 0, 0);  
    if(rshm == (void*)-1)  
    {  
        fprintf(stderr, "shmat failed\n");  
        exit(EXIT_FAILURE);  
    }  
    //printf("\n Cap Memory attached at A33 %X\n", (int)rshm); 	
    shared_cap = (struct shared_use_st*)rshm;  
	shared_cap->written = 0; 	
	
	
	
	//printf("ca_AiProc start\n");
    while (pstAiCtl->bStart)
    {     
        TimeoutVal.tv_sec = 1;
        TimeoutVal.tv_usec = 0;
        
        FD_ZERO(&read_fds);
        FD_SET(AiFd,&read_fds);
        
        s32Ret = select(AiFd+1, &read_fds, NULL, NULL, &TimeoutVal);
        if (s32Ret < 0) 
        {
            break;
        }
        else if (0 == s32Ret) 
        {
            printf("%s: get ai frame select time out\n", __FUNCTION__);
            break;
        }        
        
        if (FD_ISSET(AiFd, &read_fds))
        {
            /* get frame from ai chn */
			memset(&stAecFrm, 0, sizeof(AEC_FRAME_S));
            s32Ret = HI_MPI_AI_GetFrame(pstAiCtl->AiDev, pstAiCtl->AiChn, &stFrame, &stAecFrm, HI_FALSE);
            if (HI_SUCCESS != s32Ret )
            {
				continue;
            }
			//printf("....... enBitwidth = %d, enSoundmode = %d\r\n", stFrame.enBitwidth, stFrame.enSoundmode);
			//printf("....... Len = %d, TimeStamp = %d, Seq = %d\r\n", stFrame.u32Len, stFrame.u64TimeStamp, stFrame.u32Seq);
			//fwrite(stFrame.pVirAddr[0],  1 ,stFrame.u32Len , pfd);
            //fflush(pfd);	
			//printf("....... Len = %d, [%.2x][%.2x][%.2x][%.2x]\r\n", stFrame.u32Len, stFrame.pVirAddr[0][0],stFrame.pVirAddr[0][1],stFrame.pVirAddr[0][2],stFrame.pVirAddr[0][3]);			
#if 1
			while(shared_cap->written == 1) //0可写，不可读
			{
				if (pstAiCtl->bStart == HI_FALSE) goto capExit;
				usleep(100);
			}

			memset(shared_cap->text, 0, stFrame.u32Len);  //海思采集必须是320
			memcpy(shared_cap->text, stFrame.pVirAddr[0], stFrame.u32Len);
			shared_cap->packet_len = stFrame.u32Len;			
			
			//fwrite(shared_cap->text,  1 ,shared_cap->packet_len , pfd);
            //fflush(pfd);	
			//printf("....... Len = %d, [%.2x][%.2x][%.2x][%.2x]\r\n", shared_cap->packet_len, shared_cap->text[0],shared_cap->text[1],shared_cap->text[2],shared_cap->text[3]);
			
			shared_cap->written = 1;
#endif		

capExit:			
            /* finally you must release the stream */
            s32Ret = HI_MPI_AI_ReleaseFrame(pstAiCtl->AiDev, pstAiCtl->AiChn, &stFrame, &stAecFrm);
            if (HI_SUCCESS != s32Ret )
            {
                printf("%s: HI_MPI_AI_ReleaseFrame(%d, %d), failed with %#x!\n",\
                       __FUNCTION__, pstAiCtl->AiDev, pstAiCtl->AiChn, s32Ret);
                pstAiCtl->bStart = HI_FALSE;
                return NULL;
            }
            
        }
    }
    //fclose(pfd);
	
    //pstAiCtl->bStart = HI_FALSE;
    return NULL;
}

void *pb_AoProc(void *parg)
{
    HI_S32 s32Ret;

    SAMPLE_AO_S *pstAoCtl = (SAMPLE_AO_S *)parg;
    AUDIO_FRAME_S stFrame; 
	char play_buf[320] = {0};
	stFrame.pVirAddr[0] = play_buf;
    //static FILE    *pfd;
	//pfd = fopen("aec_test.pcm", "w+");	
	
#if 1	
	//播放
	struct shared_use_st *shared;//指向shm  
	int shmid;//共享内存标识符	
	void *shm = NULL;
#ifdef HI_AUDIO_SMM_LOOKUP_ON	
	shmid = shmget((key_t)12345, sizeof(struct shared_use_st), 0666|IPC_CREAT); 
#else
	shmid = shmget((key_t)67890, sizeof(struct shared_use_st), 0666|IPC_CREAT); 
#endif	
	
    if(shmid == -1)  
    {  
        fprintf(stderr, "shmget failed\n");  
        exit(EXIT_FAILURE);  
    }
    //将共享内存连接到当前进程的地址空间  
    shm = shmat(shmid, 0, 0);  
    if(shm == (void*)-1)  
    {  
        fprintf(stderr, "shmat failed\n");  
        exit(EXIT_FAILURE);  
    }  
    //printf("\n PB Memory attached at A33 %X\n", (int)shm); 	
    shared = (struct shared_use_st*)shm;  
	shared->written = 0; 	
#endif	
	
	//printf("pb_AoProc start\n");
    while (pstAoCtl->bStart)
    {           

		shared->written = 0;
		while(shared->written == 0)
		{
			if (pstAoCtl->bStart == HI_FALSE) goto pbExit;
			usleep(100);
		}
		
		//printf("xxxxxxx Len = %d\r\n", shared->packet_len);	//320
		
		//fwrite(shared->text,  1 ,shared->packet_len , pfd);
		//fwrite(stFrame.pVirAddr[0],  1 ,stFrame.u32Len , pfd);
		//fflush(pfd);	
		
		stFrame.u32Len = shared->packet_len;
		memcpy(stFrame.pVirAddr[0], shared->text, shared->packet_len); //320
		stFrame.enBitwidth	= 1;
		stFrame.enSoundmode	= 0;
	
#if 1
		
		/* send frame to ao */
		//printf("================ pstAoCtl->AoDev %d\n", pstAoCtl->AoDev);
		s32Ret = HI_MPI_AO_SendFrame(pstAoCtl->AoDev, pstAoCtl->AoChn, &stFrame, 1000);
		if (HI_SUCCESS != s32Ret )
		{
			printf("%s: HI_MPI_AO_SendFrame(%d, %d), failed with %#x!\n",\
				   __FUNCTION__, pstAoCtl->AoDev, pstAoCtl->AoChn, s32Ret);
			pstAoCtl->bStart = HI_FALSE;
			return NULL;
		}
#endif            

    }
pbExit:	
	//fclose(pfd);
    //pstAoCtl->bStart = HI_FALSE;
    return NULL;
}


HI_S32 SAMPLE_COMM_AUDIO_CreatTrdAiTHR(AUDIO_DEV AiDev, AI_CHN AiChn)
{
	SAMPLE_AI_S *pstAi = NULL;
	pstAi = &gs_stSampleAi[0];
    pstAi->bStart= HI_TRUE;
    pstAi->AiDev = AiDev;
    pstAi->AiChn = AiChn;

	pthread_create(&pstAi->stAiPid, 0, ca_AiProc, pstAi);
    
    return HI_SUCCESS;
}

HI_S32 SAMPLE_COMM_AUDIO_CreatTrdAoTHR(AUDIO_DEV AoDev, AO_CHN AoChn)
{
	SAMPLE_AO_S	*pstAo = NULL;
	pstAo = &gs_stSampleAo[0];
    pstAo->bStart= HI_TRUE;
    pstAo->AoDev = AoDev;
    pstAo->AoChn = AoChn;	

	pthread_create(&pstAo->stAoPid, 0, pb_AoProc, pstAo);
    
    return HI_SUCCESS;
}

HI_S32 SAMPLE_COMM_AUDIO_DestoryTrdAioTHR(AUDIO_DEV AiDev, AI_CHN AiChn, AUDIO_DEV AoDev, AO_CHN AoChn)
{
    SAMPLE_AI_S *pstAi = NULL;
    SAMPLE_AO_S	*pstAo = NULL;
    pstAi = &gs_stSampleAi[0];
	pstAo = &gs_stSampleAo[0];
    if (pstAi->bStart)
    {
        pstAi->bStart = HI_FALSE;
        pthread_join(pstAi->stAiPid, 0);
    }
    if (pstAo->bStart)
    {
        pstAo->bStart = HI_FALSE;
        pthread_join(pstAo->stAoPid, 0);
    }

    return HI_SUCCESS;
}

/******************************************************************************
* function : Create the thread to get frame from ai and send to ao
******************************************************************************/
HI_S32 SAMPLE_COMM_AUDIO_CreatTrdAiAo(AUDIO_DEV AiDev, AI_CHN AiChn, AUDIO_DEV AoDev, AO_CHN AoChn)
{
    SAMPLE_AI_S *pstAi = NULL;
    
    pstAi = &gs_stSampleAi[0]; //AiDev*AIO_MAX_CHN_NUM + AiChn
    pstAi->bSendAenc = HI_FALSE;
    pstAi->bSendAo = HI_TRUE;
    pstAi->bStart= HI_TRUE;
    pstAi->AiDev = AiDev;
    pstAi->AiChn = AiChn;
    pstAi->AoDev = AoDev;
    pstAi->AoChn = AoChn;

    pthread_create(&pstAi->stAiPid, 0, SAMPLE_COMM_AUDIO_AiProc, pstAi);
    
    return HI_SUCCESS;
}

/******************************************************************************
* function : Create the thread to get frame from ai and send to aenc
******************************************************************************/
HI_S32 SAMPLE_COMM_AUDIO_CreatTrdAiAenc(AUDIO_DEV AiDev, AI_CHN AiChn, AENC_CHN AeChn)
{
    SAMPLE_AI_S *pstAi = NULL;
    
    pstAi = &gs_stSampleAi[AiDev*AIO_MAX_CHN_NUM + AiChn];
    pstAi->bSendAenc = HI_TRUE;
    pstAi->bSendAo = HI_FALSE;
    pstAi->bStart= HI_TRUE;
    pstAi->AiDev = AiDev;
    pstAi->AiChn = AiChn;
    pstAi->AencChn = AeChn;
    pthread_create(&pstAi->stAiPid, 0, SAMPLE_COMM_AUDIO_AiProc, pstAi);
    
    return HI_SUCCESS;
}

/******************************************************************************
* function : Create the thread to get stream from aenc and send to adec
******************************************************************************/
#if 1
void *SAMPLE_COMM_AUDIO_AencProcPlay(void *parg)
{
    HI_S32 s32Ret;
    HI_S32 AencFd;
    SAMPLE_AENC_S *pstAencCtl = (SAMPLE_AENC_S *)parg;

    AUDIO_STREAM_S stStreamPlay;
	char play_buf[640] = {0};
	stStreamPlay.pStream = play_buf;
	stStreamPlay.u32Len = 4;

	//播放
	struct shared_use_st *shared;//指向shm  
	int shmid;//共享内存标识符	
	void *shm = NULL;
    shmid = shmget((key_t)12345, sizeof(struct shared_use_st), 0666|IPC_CREAT); 
    if(shmid == -1)  
    {  
        fprintf(stderr, "shmget failed\n");  
        exit(EXIT_FAILURE);  
    }
    //将共享内存连接到当前进程的地址空间  
    shm = shmat(shmid, 0, 0);  
    if(shm == (void*)-1)  
    {  
        fprintf(stderr, "shmat failed\n");  
        exit(EXIT_FAILURE);  
    }  
    printf("\nMemory attached at A33 %X\n", (int)shm); 	
    shared = (struct shared_use_st*)shm;  
	shared->written = 0; 	
	
    while (pstAencCtl->bStart)
    {     
		/* 2.播放 */
		shared->written = 0;
		while(shared->written == 0)
		{
			usleep(100);
			//printf("++++++\r\n");
		}

		//else
		{
			//可读，拷贝
			memcpy(stStreamPlay.pStream + stStreamPlay.u32Len, shared->text, shared->packet_len); //320
						
			stStreamPlay.u32Len += shared->packet_len;
			//packet_len++;
			//printf("+++++ %d\r\n",shared->packet_len);
			if (stStreamPlay.u32Len == 324) 
			{
				stStreamPlay.pStream[0] = 0;
				stStreamPlay.pStream[1] = 1;
				stStreamPlay.pStream[2] = 160;
				stStreamPlay.pStream[3] = 0;
				//printf("+++++ %d\r\n",stStreamPlay.u32Len);
				/* send stream to decoder and play for testing */
				if (HI_TRUE == pstAencCtl->bSendAdChn)
				{
					s32Ret = HI_MPI_ADEC_SendStream(pstAencCtl->AdChn, &stStreamPlay, HI_TRUE);
					if (HI_SUCCESS != s32Ret )
					{
						printf("%s: HI_MPI_ADEC_SendStream(%d), failed with %#x!\n",\
							   __FUNCTION__, pstAencCtl->AdChn, s32Ret);
						pstAencCtl->bStart = HI_FALSE;
						return NULL;
					}
				}
				//packet_len = 0;
				stStreamPlay.u32Len = 4;
				memset(stStreamPlay.pStream, 0, 640);
			}
			
		}
    }

    return NULL;
}
#else
void *SAMPLE_COMM_AUDIO_AencProcPlay(void *parg)
{
    HI_S32 s32Ret;
    HI_S32 AencFd;
    SAMPLE_AENC_S *pstAencCtl = (SAMPLE_AENC_S *)parg;

    AUDIO_STREAM_S stStreamPlay;
	char play_buf[640] = {0};
	int read_len = 0;
	stStreamPlay.pStream = play_buf;
	stStreamPlay.u32Len = 4;
	
	ghsua_shm_t *shm = NULL;
	int ret = -1;
	ret = ghsua_shm_alloc(12345, 8*1024, &shm);
	printf("ghsua_shm_alloc ret = %d\r\n", ret);
    while (pstAencCtl->bStart)
    {     
		/* 2.播放 */
		ghsua_shm_get(shm, stStreamPlay.pStream + stStreamPlay.u32Len , 320, &read_len);
		stStreamPlay.u32Len += read_len;

		//可读，拷贝
		//memcpy(stStreamPlay.pStream + stStreamPlay.u32Len, shared->text, read_len); //320
		//packet_len++;
		//printf("+++++ %d\r\n",stStreamPlay.u32Len);
		if (stStreamPlay.u32Len == 324) 
		{
			stStreamPlay.pStream[0] = 0;
			stStreamPlay.pStream[1] = 1;
			stStreamPlay.pStream[2] = 160;
			stStreamPlay.pStream[3] = 0;
			printf("+++++ %d\r\n",stStreamPlay.u32Len);
			/* send stream to decoder and play for testing */
			if (HI_TRUE == pstAencCtl->bSendAdChn)
			{
				s32Ret = HI_MPI_ADEC_SendStream(pstAencCtl->AdChn, &stStreamPlay, HI_TRUE);
				if (HI_SUCCESS != s32Ret )
				{
					printf("%s: HI_MPI_ADEC_SendStream(%d), failed with %#x!\n",\
						   __FUNCTION__, pstAencCtl->AdChn, s32Ret);
					pstAencCtl->bStart = HI_FALSE;
					return NULL;
				}
			}
			//packet_len = 0;
			//read_len = 0;
			stStreamPlay.u32Len = 4;
			memset(stStreamPlay.pStream, 0, 640);
		}			

		
		usleep(19*1000);  //40ms 25fps
    }

    return NULL;
}
#endif
HI_S32 SAMPLE_COMM_AUDIO_CreatTrdAencAdec(AENC_CHN AeChn, ADEC_CHN AdChn, FILE *pAecFd)
{
    SAMPLE_AENC_S *pstAenc = NULL;

    if (NULL == pAecFd)
    {
        return HI_FAILURE;
    }
    
    pstAenc = &gs_stSampleAenc[AeChn];
    pstAenc->AeChn = AeChn;
    pstAenc->AdChn = AdChn;
    pstAenc->bSendAdChn = HI_TRUE;
    pstAenc->pfd = pAecFd;    
    pstAenc->bStart = HI_TRUE;


    pthread_create(&pstAenc->stAencPid, 0, SAMPLE_COMM_AUDIO_AencProc, pstAenc);
	
	pthread_create(&pstAenc->stAencPidPlay, 0, SAMPLE_COMM_AUDIO_AencProcPlay, pstAenc);
    
    return HI_SUCCESS;
}

/******************************************************************************
* function : Create the thread to get stream from file and send to adec
******************************************************************************/
HI_S32 SAMPLE_COMM_AUDIO_CreatTrdFileAdec(ADEC_CHN AdChn, FILE *pAdcFd)
{
    SAMPLE_ADEC_S *pstAdec = NULL;

    if (NULL == pAdcFd)
    {
        return HI_FAILURE;
    }

    pstAdec = &gs_stSampleAdec[AdChn];
    pstAdec->AdChn = AdChn;
    pstAdec->pfd = pAdcFd;
    pstAdec->bStart = HI_TRUE;
    pthread_create(&pstAdec->stAdPid, 0, SAMPLE_COMM_AUDIO_AdecProc, pstAdec);
    
    return HI_SUCCESS;
}


/******************************************************************************
* function : Create the thread to set Ao volume 
******************************************************************************/
HI_S32 SAMPLE_COMM_AUDIO_CreatTrdAoVolCtrl(AUDIO_DEV AoDev)
{
	SAMPLE_AO_S *pstAoCtl = NULL;
	
	pstAoCtl =  &gs_stSampleAo[AoDev];
	pstAoCtl->AoDev =  AoDev;
	pstAoCtl->bStart = HI_TRUE;
	pthread_create(&pstAoCtl->stAoPid, 0, SAMPLE_COMM_AUDIO_AoVolProc, pstAoCtl);

    return HI_SUCCESS;
}


/******************************************************************************
* function : Destory the thread to get frame from ai and send to ao or aenc
******************************************************************************/
HI_S32 SAMPLE_COMM_AUDIO_DestoryTrdAi(AUDIO_DEV AiDev, AI_CHN AiChn)
{
    SAMPLE_AI_S *pstAi = NULL;
    
    pstAi = &gs_stSampleAi[AiDev*AIO_MAX_CHN_NUM + AiChn];
    if (pstAi->bStart)
    {
        pstAi->bStart = HI_FALSE;
        //pthread_cancel(pstAi->stAiPid);
        pthread_join(pstAi->stAiPid, 0);
    }


    return HI_SUCCESS;
}

/******************************************************************************
* function : Destory the thread to get stream from aenc and send to adec
******************************************************************************/
HI_S32 SAMPLE_COMM_AUDIO_DestoryTrdAencAdec(AENC_CHN AeChn)
{
    SAMPLE_AENC_S *pstAenc = NULL;

    pstAenc = &gs_stSampleAenc[AeChn];
    if (pstAenc->bStart)
    {
        pstAenc->bStart = HI_FALSE;
        //pthread_cancel(pstAenc->stAencPid);
        pthread_join(pstAenc->stAencPid, 0);
		pthread_join(pstAenc->stAencPidPlay, 0);
    }


    return HI_SUCCESS;
}

/******************************************************************************
* function : Destory the thread to get stream from file and send to adec
******************************************************************************/
HI_S32 SAMPLE_COMM_AUDIO_DestoryTrdFileAdec(ADEC_CHN AdChn)
{
    SAMPLE_ADEC_S *pstAdec = NULL;

    pstAdec = &gs_stSampleAdec[AdChn];
    if (pstAdec->bStart)
    {
        pstAdec->bStart = HI_FALSE;
        //pthread_cancel(pstAdec->stAdPid);
        pthread_join(pstAdec->stAdPid, 0);
    }


    return HI_SUCCESS;
}

/******************************************************************************
* function : Destory the thread to set Ao volume 
******************************************************************************/
HI_S32 SAMPLE_COMM_AUDIO_DestoryTrdAoVolCtrl(AUDIO_DEV AoDev)
{
    SAMPLE_AO_S* pstAoCtl = NULL;

    pstAoCtl =  &gs_stSampleAo[AoDev];
    if (pstAoCtl->bStart)
    {
        pstAoCtl->bStart = HI_FALSE;
        pthread_cancel(pstAoCtl->stAoPid);
        pthread_join(pstAoCtl->stAoPid, 0);
    }


    return HI_SUCCESS;
}

/******************************************************************************
* function : Ao bind Adec
******************************************************************************/
HI_S32 SAMPLE_COMM_AUDIO_AoBindAdec(AUDIO_DEV AoDev, AO_CHN AoChn, ADEC_CHN AdChn)
{
    MPP_CHN_S stSrcChn,stDestChn;

    stSrcChn.enModId = HI_ID_ADEC;
    stSrcChn.s32DevId = 0;
    stSrcChn.s32ChnId = AdChn;
    stDestChn.enModId = HI_ID_AO;
    stDestChn.s32DevId = AoDev;
    stDestChn.s32ChnId = AoChn;
    
    return HI_MPI_SYS_Bind(&stSrcChn, &stDestChn); 
}

/******************************************************************************
* function : Ao unbind Adec
******************************************************************************/
HI_S32 SAMPLE_COMM_AUDIO_AoUnbindAdec(AUDIO_DEV AoDev, AO_CHN AoChn, ADEC_CHN AdChn)
{
    MPP_CHN_S stSrcChn,stDestChn;

    stSrcChn.enModId = HI_ID_ADEC;
    stSrcChn.s32ChnId = AdChn;
    stSrcChn.s32DevId = 0;
    stDestChn.enModId = HI_ID_AO;
    stDestChn.s32DevId = AoDev;
    stDestChn.s32ChnId = AoChn;
    
    return HI_MPI_SYS_UnBind(&stSrcChn, &stDestChn); 
}

/******************************************************************************
* function : Ao bind Ai
******************************************************************************/
HI_S32 SAMPLE_COMM_AUDIO_AoBindAi(AUDIO_DEV AiDev, AI_CHN AiChn, AUDIO_DEV AoDev, AO_CHN AoChn)
{
    MPP_CHN_S stSrcChn,stDestChn;

    stSrcChn.enModId = HI_ID_AI;
    stSrcChn.s32ChnId = AiChn;
    stSrcChn.s32DevId = AiDev;
    stDestChn.enModId = HI_ID_AO;
    stDestChn.s32DevId = AoDev;
    stDestChn.s32ChnId = AoChn;
    
    return HI_MPI_SYS_Bind(&stSrcChn, &stDestChn); 
}

/******************************************************************************
* function : Ao unbind Ai
******************************************************************************/
HI_S32 SAMPLE_COMM_AUDIO_AoUnbindAi(AUDIO_DEV AiDev, AI_CHN AiChn, AUDIO_DEV AoDev, AO_CHN AoChn)
{
    MPP_CHN_S stSrcChn,stDestChn;

    stSrcChn.enModId = HI_ID_AI;
    stSrcChn.s32ChnId = AiChn;
    stSrcChn.s32DevId = AiDev;
    stDestChn.enModId = HI_ID_AO;
    stDestChn.s32DevId = AoDev;
    stDestChn.s32ChnId = AoChn;
    
    return HI_MPI_SYS_UnBind(&stSrcChn, &stDestChn); 
}

/******************************************************************************
* function : Aenc bind Ai
******************************************************************************/
HI_S32 SAMPLE_COMM_AUDIO_AencBindAi(AUDIO_DEV AiDev, AI_CHN AiChn, AENC_CHN AeChn)
{
    MPP_CHN_S stSrcChn,stDestChn;

    stSrcChn.enModId = HI_ID_AI;
    stSrcChn.s32DevId = AiDev;
    stSrcChn.s32ChnId = AiChn;
    stDestChn.enModId = HI_ID_AENC;
    stDestChn.s32DevId = 0;
    stDestChn.s32ChnId = AeChn;
    
    return HI_MPI_SYS_Bind(&stSrcChn, &stDestChn);
}

/******************************************************************************
* function : Aenc unbind Ai
******************************************************************************/
HI_S32 SAMPLE_COMM_AUDIO_AencUnbindAi(AUDIO_DEV AiDev, AI_CHN AiChn, AENC_CHN AeChn)
{
    MPP_CHN_S stSrcChn,stDestChn;

    stSrcChn.enModId = HI_ID_AI;
    stSrcChn.s32DevId = AiDev;
    stSrcChn.s32ChnId = AiChn;
    stDestChn.enModId = HI_ID_AENC;
    stDestChn.s32DevId = 0;
    stDestChn.s32ChnId = AeChn;
    
    return HI_MPI_SYS_UnBind(&stSrcChn, &stDestChn);      
}

#if 0
/******************************************************************************
* function : Acodec config [ s32Samplerate(0:8k, 1:16k ) ]
******************************************************************************/
HI_S32 SAMPLE_COMM_AUDIO_CfgAcodec(AUDIO_SAMPLE_RATE_E enSample, HI_BOOL bMicIn)
{
    HI_S32 fdAcodec = -1;
    ACODEC_CTRL stAudioctrl = {0};

    fdAcodec = open(ACODEC_FILE,O_RDWR);
    if (fdAcodec < 0)
    {
        printf("%s: can't open acodec,%s\n", __FUNCTION__, ACODEC_FILE);
        return HI_FAILURE;     
    }

    if ((AUDIO_SAMPLE_RATE_8000 == enSample)
        || (AUDIO_SAMPLE_RATE_11025 == enSample)
        || (AUDIO_SAMPLE_RATE_12000 == enSample))
    {
        stAudioctrl.i2s_fs_sel = 0x18;
    }
    else if ((AUDIO_SAMPLE_RATE_16000 == enSample)
        || (AUDIO_SAMPLE_RATE_22050 == enSample)
        || (AUDIO_SAMPLE_RATE_24000 == enSample))
    {
        stAudioctrl.i2s_fs_sel = 0x19;
    }
    else if ((AUDIO_SAMPLE_RATE_32000 == enSample)
        || (AUDIO_SAMPLE_RATE_44100 == enSample)
        || (AUDIO_SAMPLE_RATE_48000 == enSample))
    {
        stAudioctrl.i2s_fs_sel = 0x1a;
    }
    else 
    {
        printf("%s: not support enSample:%d\n", __FUNCTION__, enSample);
        return HI_FAILURE;
    }
    
    if (ioctl(fdAcodec, ACODEC_SET_I2S1_FS, &stAudioctrl))
    {
        printf("%s: set acodec sample rate failed\n", __FUNCTION__);
        return HI_FAILURE;
    }

    if (HI_TRUE == bMicIn)
    {
        stAudioctrl.mixer_mic_ctrl = ACODEC_MIXER_MICIN;
        if (ioctl(fdAcodec, ACODEC_SET_MIXER_MIC, &stAudioctrl))
        {
            printf("%s: set acodec micin failed\n", __FUNCTION__);
            return HI_FAILURE;
        }

        /* set volume plus (0~0x1f,default 0) */
        stAudioctrl.gain_mic = 0;
        if (ioctl(fdAcodec, ACODEC_SET_GAIN_MICL, &stAudioctrl))
        {
            printf("%s: set acodec micin volume failed\n", __FUNCTION__);
            return HI_FAILURE;
        }
        if (ioctl(fdAcodec, ACODEC_SET_GAIN_MICR, &stAudioctrl))
        {
            printf("%s: set acodec micin volume failed\n", __FUNCTION__);
            return HI_FAILURE;
        }
    }
    close(fdAcodec);    

    return HI_SUCCESS;
}

/******************************************************************************
* function : Disable Tlv320
******************************************************************************/
HI_S32 SAMPLE_COMM_AUDIO_DisableAcodec()
{
    return  SAMPLE_COMM_AUDIO_CfgAcodec(AUDIO_SAMPLE_RATE_48000, HI_FALSE);
}

#endif

/******************************************************************************
* function : Start Ai
******************************************************************************/
HI_S32 SAMPLE_COMM_AUDIO_StartAi(AUDIO_DEV AiDevId, HI_S32 s32AiChnCnt,
                                 AIO_ATTR_S* pstAioAttr, AUDIO_SAMPLE_RATE_E enOutSampleRate, HI_BOOL bResampleEn, HI_VOID* pstAiVqeAttr, HI_U32 u32AiVqeType)
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
    }
    }
    
    return HI_SUCCESS;
}

/******************************************************************************
* function : Stop Ai
******************************************************************************/
HI_S32 SAMPLE_COMM_AUDIO_StopAi(AUDIO_DEV AiDevId, HI_S32 s32AiChnCnt,
        HI_BOOL bResampleEn, HI_BOOL bVqeEn)
{
    HI_S32 i; 
    HI_S32 s32Ret;
    
    for (i=0; i<s32AiChnCnt; i++)
    {
        if (HI_TRUE == bResampleEn)
        {
            s32Ret = HI_MPI_AI_DisableReSmp(AiDevId, i);
            if(HI_SUCCESS != s32Ret)
            {
                printf("[Func]:%s [Line]:%d [Info]:%s\n", __FUNCTION__, __LINE__, "failed");
                return s32Ret;
            }
        }

		if (HI_TRUE == bVqeEn)
        {
            s32Ret = HI_MPI_AI_DisableVqe(AiDevId, i);
            if(HI_SUCCESS != s32Ret)
            {
                printf("[Func]:%s [Line]:%d [Info]:%s\n", __FUNCTION__, __LINE__, "failed");
                return s32Ret;
            }
        }
        
        s32Ret = HI_MPI_AI_DisableChn(AiDevId, i);
        if(HI_SUCCESS != s32Ret)
        {
            printf("[Func]:%s [Line]:%d [Info]:%s\n", __FUNCTION__, __LINE__, "failed");
            return s32Ret;
        }
    }  
    
    s32Ret = HI_MPI_AI_Disable(AiDevId);
    if(HI_SUCCESS != s32Ret)
    {
        printf("[Func]:%s [Line]:%d [Info]:%s\n", __FUNCTION__, __LINE__, "failed");
        return s32Ret;
    }
    
    return HI_SUCCESS;
}


/******************************************************************************
* function : Start Ao
******************************************************************************/
HI_S32 SAMPLE_COMM_AUDIO_StartAo(AUDIO_DEV AoDevId, HI_S32 s32AoChnCnt,
                                 AIO_ATTR_S* pstAioAttr, AUDIO_SAMPLE_RATE_E enInSampleRate, HI_BOOL bResampleEn, HI_VOID* pstAoVqeAttr, HI_U32 u32AoVqeType)
{
	HI_S32 i;
    HI_S32 s32Ret;

    s32Ret = HI_MPI_AO_SetPubAttr(AoDevId, pstAioAttr);
    if(HI_SUCCESS != s32Ret)
    {
        printf("%s: HI_MPI_AO_SetPubAttr(%d) failed with %#x!\n", __FUNCTION__, \
               AoDevId,s32Ret);
        return HI_FAILURE;
    }
    
    s32Ret = HI_MPI_AO_Enable(AoDevId);
    if(HI_SUCCESS != s32Ret)
    {
        printf("%s: HI_MPI_AO_Enable(%d) failed with %#x!\n", __FUNCTION__, AoDevId, s32Ret);
        return HI_FAILURE;
    }

	for (i=0; i<s32AoChnCnt; i++)
    {
        s32Ret = HI_MPI_AO_EnableChn(AoDevId, i/(pstAioAttr->enSoundmode + 1));
        if (HI_SUCCESS != s32Ret)
        {
            printf("%s: HI_MPI_AO_EnableChn(%d) failed with %#x!\n", __FUNCTION__, i, s32Ret);
            return HI_FAILURE;
        }

        if (HI_TRUE == bResampleEn)
        {
            s32Ret = HI_MPI_AO_DisableReSmp(AoDevId, i);
            s32Ret |= HI_MPI_AO_EnableReSmp(AoDevId, i, enInSampleRate);
            if (HI_SUCCESS != s32Ret)
            {
                printf("%s: HI_MPI_AO_EnableReSmp(%d,%d) failed with %#x!\n", __FUNCTION__, AoDevId, i, s32Ret);
                return HI_FAILURE;
            }
        }
		if (NULL != pstAoVqeAttr)
        {
			HI_BOOL bAoVqe = HI_TRUE;
			switch (u32AoVqeType)
            {
				case 0:
                    s32Ret = HI_SUCCESS;
					bAoVqe = HI_FALSE;
                    break;
                case 1:
                    s32Ret = HI_MPI_AO_SetVqeAttr(AoDevId, i, (AO_VQE_CONFIG_S *)pstAoVqeAttr);
                    break;
                default:
                    s32Ret = HI_FAILURE;
                    break;
    }
            if (s32Ret)
            {
                printf("%s: SetAoVqe%d(%d,%d) failed with %#x\n", __FUNCTION__, u32AoVqeType, AoDevId, i, s32Ret);
                return s32Ret;
            }
		    if (bAoVqe)
            {
                s32Ret = HI_MPI_AO_EnableVqe(AoDevId, i);
	            if (s32Ret)
	            {
	                printf("%s: HI_MPI_AI_EnableVqe(%d,%d) failed with %#x\n", __FUNCTION__, AoDevId, i, s32Ret);
	                return s32Ret;
	            }
            }
        }
    }
    return HI_SUCCESS;
}

/******************************************************************************
* function : Stop Ao
******************************************************************************/
HI_S32 SAMPLE_COMM_AUDIO_StopAo(AUDIO_DEV AoDevId, HI_S32 s32AoChnCnt, HI_BOOL bResampleEn, HI_BOOL bVqeEn)
{
	HI_S32 i;
    HI_S32 s32Ret;

	for (i=0; i<s32AoChnCnt; i++)
    {
	    if (HI_TRUE == bResampleEn)
	    {
	        s32Ret = HI_MPI_AO_DisableReSmp(AoDevId, i);
	        if (HI_SUCCESS != s32Ret)
	        {
	            printf("%s: HI_MPI_AO_DisableReSmp failed with %#x!\n", __FUNCTION__, s32Ret);
	            return s32Ret;
	        }
	    }

		if (HI_TRUE == bVqeEn)
        {
            s32Ret = HI_MPI_AO_DisableVqe(AoDevId, i);
            if (HI_SUCCESS != s32Ret)
            {
                printf("[Func]:%s [Line]:%d [Info]:%s\n", __FUNCTION__, __LINE__, "failed");
                return s32Ret;
            }
        }
	    s32Ret = HI_MPI_AO_DisableChn(AoDevId, i);
	    if (HI_SUCCESS != s32Ret)
	    {
	        printf("%s: HI_MPI_AO_DisableChn failed with %#x!\n", __FUNCTION__, s32Ret);
	        return s32Ret;
	    }
	}
	
    s32Ret = HI_MPI_AO_Disable(AoDevId);
    if (HI_SUCCESS != s32Ret)
    {
        printf("%s: HI_MPI_AO_Disable failed with %#x!\n", __FUNCTION__, s32Ret);
        return s32Ret;
    }

    return HI_SUCCESS;
}

/******************************************************************************
* function : Start Aenc
******************************************************************************/
HI_S32 SAMPLE_COMM_AUDIO_StartAenc(HI_S32 s32AencChnCnt, HI_U32 u32AencPtNumPerFrm, PAYLOAD_TYPE_E enType)
{
    AENC_CHN AeChn;
    HI_S32 s32Ret, i;
    AENC_CHN_ATTR_S stAencAttr;
    AENC_ATTR_ADPCM_S stAdpcmAenc;
    AENC_ATTR_G711_S stAencG711;
    AENC_ATTR_G726_S stAencG726;
    AENC_ATTR_LPCM_S stAencLpcm;
    
    /* set AENC chn attr */
    
    stAencAttr.enType = enType;
    stAencAttr.u32BufSize = 30;
    stAencAttr.u32PtNumPerFrm = u32AencPtNumPerFrm;
		
    if (PT_ADPCMA == stAencAttr.enType)
    {
        stAencAttr.pValue       = &stAdpcmAenc;
        stAdpcmAenc.enADPCMType = AUDIO_ADPCM_TYPE;
    }
    else if (PT_G711A == stAencAttr.enType || PT_G711U == stAencAttr.enType)
    {
        stAencAttr.pValue       = &stAencG711;
    }
    else if (PT_G726 == stAencAttr.enType)
    {
        stAencAttr.pValue       = &stAencG726;
        stAencG726.enG726bps    = G726_BPS;
    }
    else if (PT_LPCM == stAencAttr.enType)
    {
        stAencAttr.pValue = &stAencLpcm;
    }
    else
    {
        printf("%s: invalid aenc payload type:%d\n", __FUNCTION__, stAencAttr.enType);
        return HI_FAILURE;
    }    

    for (i=0; i<s32AencChnCnt; i++)
    {            
        AeChn = i;
        
        /* create aenc chn*/
        s32Ret = HI_MPI_AENC_CreateChn(AeChn, &stAencAttr);
        if (HI_SUCCESS != s32Ret)
        {
            printf("%s: HI_MPI_AENC_CreateChn(%d) failed with %#x!\n", __FUNCTION__,
                   AeChn, s32Ret);
            return s32Ret;
        }        
    }
    
    return HI_SUCCESS;
}

/******************************************************************************
* function : Stop Aenc
******************************************************************************/
HI_S32 SAMPLE_COMM_AUDIO_StopAenc(HI_S32 s32AencChnCnt)
{
    HI_S32 i;
    HI_S32 s32Ret;
    
    for (i=0; i<s32AencChnCnt; i++)
    {
        s32Ret = HI_MPI_AENC_DestroyChn(i);
        if (HI_SUCCESS != s32Ret)
        {
            printf("%s: HI_MPI_AENC_DestroyChn(%d) failed with %#x!\n", __FUNCTION__,
                   i, s32Ret);
            return s32Ret;
        }
        
    }
    
    return HI_SUCCESS;
}

/******************************************************************************
* function : Destory the all thread
******************************************************************************/
HI_S32 SAMPLE_COMM_AUDIO_DestoryAllTrd()
{
    HI_U32 u32DevId, u32ChnId;

    for (u32DevId = 0; u32DevId < AI_DEV_MAX_NUM; u32DevId ++)
    {
        for (u32ChnId = 0; u32ChnId < AIO_MAX_CHN_NUM; u32ChnId ++)
        {
            if(HI_SUCCESS != SAMPLE_COMM_AUDIO_DestoryTrdAi(u32DevId, u32ChnId))
            {
                printf("%s: SAMPLE_COMM_AUDIO_DestoryTrdAi(%d,%d) failed!\n", __FUNCTION__,
                   u32DevId, u32ChnId);
                return HI_FAILURE;
            }
        }
    }

    for (u32ChnId = 0; u32ChnId < AENC_MAX_CHN_NUM; u32ChnId ++)
    {
        if (HI_SUCCESS != SAMPLE_COMM_AUDIO_DestoryTrdAencAdec(u32ChnId))
        {
            printf("%s: SAMPLE_COMM_AUDIO_DestoryTrdAencAdec(%d) failed!\n", __FUNCTION__,
               u32ChnId);
            return HI_FAILURE;
        }
    }

    for (u32ChnId = 0; u32ChnId < ADEC_MAX_CHN_NUM; u32ChnId ++)
    {
        if (HI_SUCCESS != SAMPLE_COMM_AUDIO_DestoryTrdFileAdec(u32ChnId))
        {
            printf("%s: SAMPLE_COMM_AUDIO_DestoryTrdFileAdec(%d) failed!\n", __FUNCTION__,
               u32ChnId);
            return HI_FAILURE;
        } 
    }

    for (u32ChnId = 0; u32ChnId < AO_DEV_MAX_NUM; u32ChnId ++)
    {
        if (HI_SUCCESS != SAMPLE_COMM_AUDIO_DestoryTrdAoVolCtrl(u32ChnId))
        {
            printf("%s: SAMPLE_COMM_AUDIO_DestoryTrdAoVolCtrl(%d) failed!\n", __FUNCTION__,
               u32ChnId);
            return HI_FAILURE;
        }   
    }


    return HI_SUCCESS;
}


/******************************************************************************
* function : Start Adec
******************************************************************************/
HI_S32 SAMPLE_COMM_AUDIO_StartAdec(ADEC_CHN AdChn, PAYLOAD_TYPE_E enType)
{
    HI_S32 s32Ret;
    ADEC_CHN_ATTR_S stAdecAttr;
    ADEC_ATTR_ADPCM_S stAdpcm;
    ADEC_ATTR_G711_S stAdecG711;
    ADEC_ATTR_G726_S stAdecG726;
    ADEC_ATTR_LPCM_S stAdecLpcm;
    
    stAdecAttr.enType = enType;
    stAdecAttr.u32BufSize = 20;
    stAdecAttr.enMode = ADEC_MODE_STREAM;/* propose use pack mode in your app */
        
    if (PT_ADPCMA == stAdecAttr.enType)
    {
        stAdecAttr.pValue = &stAdpcm;
        stAdpcm.enADPCMType = AUDIO_ADPCM_TYPE ;
    }
    else if (PT_G711A == stAdecAttr.enType || PT_G711U == stAdecAttr.enType)
    {
        stAdecAttr.pValue = &stAdecG711;
    }
    else if (PT_G726 == stAdecAttr.enType)
    {
        stAdecAttr.pValue = &stAdecG726;
        stAdecG726.enG726bps = G726_BPS ;      
    }
    else if (PT_LPCM == stAdecAttr.enType)
    {
        stAdecAttr.pValue = &stAdecLpcm;
        stAdecAttr.enMode = ADEC_MODE_PACK;/* lpcm must use pack mode */
    }
    else
    {
        printf("%s: invalid aenc payload type:%d\n", __FUNCTION__, stAdecAttr.enType);
        return HI_FAILURE;
    }     
    
    /* create adec chn*/
    s32Ret = HI_MPI_ADEC_CreateChn(AdChn, &stAdecAttr);
    if (HI_SUCCESS != s32Ret)
    {
        printf("%s: HI_MPI_ADEC_CreateChn(%d) failed with %#x!\n", __FUNCTION__,\
               AdChn,s32Ret);
        return s32Ret;
    }
    return 0;
}

/******************************************************************************
* function : Stop Adec
******************************************************************************/
HI_S32 SAMPLE_COMM_AUDIO_StopAdec(ADEC_CHN AdChn)
{
    HI_S32 s32Ret;
    
    s32Ret = HI_MPI_ADEC_DestroyChn(AdChn);
    if (HI_SUCCESS != s32Ret)
    {
        printf("%s: HI_MPI_ADEC_DestroyChn(%d) failed with %#x!\n", __FUNCTION__,
               AdChn, s32Ret);
        return s32Ret;
    }
    
    return HI_SUCCESS;
}

/******************************************************************************
* function : set Ai Vol(-20,10)
******************************************************************************/
HI_S32 SAMPLE_COMM_AUDIO_SetAiVolume(AUDIO_DEV AiDev, AI_CHN AiChn, HI_S32 s32Volume)
{
    HI_S32 s32Ret;
    
	s32Ret = HI_MPI_AI_SetVqeVolume( AiDev, AiChn, s32Volume);
	if(HI_SUCCESS != s32Ret)
	{
		printf("%s: HI_MPI_AI_SetVqeVolume(%d), failed with %#x!\n",\
			   __FUNCTION__, AiDev, s32Ret);
	}
    
    return HI_SUCCESS;
}
/******************************************************************************
* function : set Ao Vol (-121,6)
******************************************************************************/
HI_S32 SAMPLE_COMM_AUDIO_SetAoVolume(AUDIO_DEV AoDev, HI_S32 s32Volume)
{
    HI_S32 s32Ret;
    
	s32Ret = HI_MPI_AO_SetVolume( AoDev, s32Volume);
	if(HI_SUCCESS != s32Ret)
	{
		printf("%s: HI_MPI_AO_SetVolume(%d), failed with %#x!\n",\
			   __FUNCTION__, AoDev, s32Ret);
	}
    
    return HI_SUCCESS;
}


#ifdef __cplusplus
#if __cplusplus
}
#endif 
#endif /* End of #ifdef __cplusplus */

