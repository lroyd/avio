/*************************************************************************
	> File Name: 
	> Author: lroyd
	> Mail: htzhangxmu@163.com
	> Created Time: 
 ************************************************************************/
#ifndef __AVIO_API_H__
#define __AVIO_API_H__




typedef int (*HI_AUDIO_CBK)(char *, int);
typedef int (*HI_VIDEO_CBK)(int, char *, int, unsigned long long);
typedef int (*HI_VIDEO_CANCEL)(void *);	//给用户提供回调取消点

int HI_AVIO_LoadConfig(const char *_pConfigPath);

void HI_AVIO_SignalHandle(int);
int HI_AVIO_Init(void);
int HI_AVIO_Deinit(void);


//


typedef enum{
    VIDEO_SER_USR = 0,
	VIDEO_SER_SAVE,
	VIDEO_SER_RTMP,		//
	VIDEO_SER_RTSP,
	VIDEO_SER_P2P,
	
	VIDEO_SER_BUTT
}VIDEO_SERVER_TYPE;

//不支持动态注册,重新注册必须先HI_AVIO_VideoStopChannel(可以加 在SAMPLE_PROC_VIDEO_RegisterServer判断)
//支持同一类型的重复注册
//每个通道最多支持4个服务
//取消点只有阻塞需要设置
int HI_AVIO_VideoRegisterServer(int _s32Chnnl, VIDEO_SERVER_TYPE _emServer, HI_VIDEO_CBK _pVideoServer, HI_VIDEO_CANCEL _pCancel, void *_pArg);

int HI_AVIO_VideoSaveFile(void); //专门为VIDEO_SER_SAVE准备，用户也可以的自定义

int HI_AVIO_VideoStartChannel(int _s32Chnnl);	//通道数1+
int HI_AVIO_VideoStopChannel(int _s32Chnnl);


/* ========================================================= */
int HI_AVIO_AudioSStart(HI_AUDIO_CBK, HI_AUDIO_CBK);	//注意：播放和采集需要对外部应用做阻塞控制
int HI_AVIO_AudioSStop(void);
int HI_AVIO_AudioSetPlayVolume(char);
int HI_AVIO_AudioPlayImmt(char *, int);



#endif


