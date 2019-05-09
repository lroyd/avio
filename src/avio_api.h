/*************************************************************************
	> File Name: 
	> Author: lroyd
	> Mail: htzhangxmu@163.com
	> Created Time: 
 ************************************************************************/
#ifndef __AVIO_API_H__
#define __AVIO_API_H__

#define	HI_AUDIO_SAMPLE_RATE		(8000)	//采样率
#define	HI_AUDIO_BIT_WIDTH			(1)		//8:0 16:1
#define HI_AUDIO_PTNUMPERFRM		(160)

typedef struct _tagAudioParamInfo
{
    int				m_u32Sample;
	int				m_u32PtNumPerfrm;
    unsigned char	m_u32BitWidth;
}T_AudioParamInfo;



#define	HI_VIDEO_CHNNL_NUM			(3)	//视频通道路数 1/2/3(固定不可修改)

typedef struct _tagVpssCrop
{
	unsigned char   m_bEnable;
    int				m_u32X;
	int				m_u32Y;
    int				m_u32W;
	int				m_u32H;	
}T_VpssCropInfo;



typedef int (*HI_AUDIO_CBK)(char *, int);
typedef int (*HI_VIDEO_CBK)(int, char *, int, unsigned long long);
typedef int (*HI_VIDEO_CANCEL)(void *);	//给用户提供回调取消点

void HI_AVIO_SignalHandle(int);
int HI_AVIO_Init(void);
int HI_AVIO_Deinit(void);

int HI_AVIO_VideoSStartChannel(int _s32Chnnl, HI_VIDEO_CBK _pVideoCbk);	//通道数1+
int HI_AVIO_VideoSetTestCancel(int _s32Chnnl, HI_VIDEO_CANCEL _pCancel, void *_pArg);	//多次调用会覆盖,使用时不是线程安全
int HI_AVIO_VideoSStopChannel(int _s32Chnnl);


/* ========================================================= */
int HI_AVIO_AudioSStart(HI_AUDIO_CBK, HI_AUDIO_CBK);	//注意：播放和采集需要对外部应用做阻塞控制
int HI_AVIO_AudioSStop(void);
int HI_AVIO_AudioSetPlayVolume(char);
int HI_AVIO_AudioPlayImmt(char *, int);



#endif


