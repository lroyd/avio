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

void HI_AVIO_SignalHandle(int);
int HI_AVIO_Init(void);
int HI_AVIO_Deinit(void);

int HI_AVIO_VideoSStartChannel(int _s32Chnnl, HI_VIDEO_CBK _pVideoCbk);	//通道数1+
int HI_AVIO_VideoSStopChannel(int _s32Chnnl);


int HI_AVIO_AudioSStart(HI_AUDIO_CBK, HI_AUDIO_CBK);	//注意：播放和采集需要对外部应用做阻塞控制
int HI_AVIO_AudioSStop(void);
int HI_AVIO_AudioSetPlayVolume(char);
int HI_AVIO_AudioPlayImmt(char *, int);



#endif


