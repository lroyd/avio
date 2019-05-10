/*************************************************************************
	> File Name: 
	> Author: lroyd
	> Mail: htzhangxmu@163.com
	> Created Time: 
 ************************************************************************/
#ifndef __CONFIG_AVIO_H__
#define __CONFIG_AVIO_H__


#define	HI_AUDIO_SAMPLE_RATE		(8000)	//采样率
#define	HI_AUDIO_BIT_WIDTH			(1)		//8:0 16:1
#define HI_AUDIO_PTNUMPERFRM		(160)

typedef struct _tagAudioParam
{
    int				m_u32Sample;
	int				m_u32PtNumPerfrm;
    unsigned char	m_u32BitWidth;
}T_AudioParam;


typedef struct _tagAudioConfigInfo
{
	unsigned char	m_bEnable:1;
	unsigned char	m_bEC:1;
	unsigned char	m_bLookup:1;
	
	unsigned char	m_bAiSave:1;
	unsigned char	m_bAoSave:1;	
	
	int				m_s32AiVolume;
	int				m_s32AoVolume;
	
	T_AudioParam	in_tAudio;	//目前没有使用是固定的{8000, 160, 1}
	
}T_AudioConfigInfo;


#define	HI_VIDEO_CHNNL_NUM			(3)	//视频通道路数 1/2/3(固定不可修改)

typedef struct _tagVpssCrop
{
	unsigned char   m_bEnable;
    int				m_u32X;
	int				m_u32Y;
    int				m_u32W;
	int				m_u32H;	
}T_VpssCropInfo;

typedef struct _tagVideoConfigInfo
{
	unsigned char	m_bEnable:1;
	unsigned char	m_bSave:1;
	
	int				m_s32GroupSize;
	
	T_VpssCropInfo	in_tCrop;
	
}T_VideoConfigInfo;


////////////////////////////////////////////////////////////////////////
//用户配置层
#define HI_AVIO_AUDIO_ON
	//音频参数设置(无编码)
	#define HI_AUDIO_AO_VOLUME			(6)		//音量输出增益-15,6
	#define HI_AUDIO_AI_VOLUME			(0)	//音量输入增益-20,10
	
	#define HI_AUDIO_EC_ON						//是否开启回声抑制(默认开启)
	#define HI_AUDIO_LOOKUP_ON					//本地回环测试(默认关闭)
	//#define HI_AUDIO_CAP_SAVE_FILE_ON		//音频的文件，不影响其他逻辑
	//#define HI_AUDIO_PLAY_SAVE_FILE_ON	

#define HI_AVIO_VIDEO_ON
	/*************************************************************/
	//#define HI_VIDEO_VENC_SAVE_FILE_ON		//视频文件存储,会关闭其他逻辑
	/*************************************************************/
	//#define	HI_VIDEO_GROUP_CROP			//-offline 视频裁剪属性，必须小于HI_VIDEO_GROUP_SIZE
	/*************************************************************/
	#define HI_VI_CHNNL_1_ON		//启用通道1,只能放大
	//#define HI_VI_CHNNL_2_ON		//启用通道2,只能缩小
	//#define HI_VI_CHNNL_3_ON		//启用通道3
	



	/*************************************************************/
	#define USE_SIMPLE_THREAD		
	



	
	
	
	
#endif


