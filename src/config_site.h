/*************************************************************************
	> File Name: 
	> Author: lroyd
	> Mail: htzhangxmu@163.com
	> Created Time: 
 ************************************************************************/
#ifndef __CONFIG_AVIO_H__
#define __CONFIG_AVIO_H__


//#define INIPARSE_USE_ON		//使用ini动态解析
#ifndef INIPARSE_USE_ON
	#define HI_AVIO_AUDIO_ON
	//音频参数设置(无编码)
	#define HI_AUDIO_AO_VOLUME			(6)		//音量输出增益-15,6
	#define HI_AUDIO_AI_VOLUME			(0)	//音量输入增益-20,10
	
	//#define HI_AUDIO_EC_ON						//是否开启回声抑制(默认开启)
	#define HI_AUDIO_LOOKUP_ON					//本地回环测试(默认关闭)
	//#define HI_AUDIO_CAP_SAVE_FILE_ON		//音频的文件，不影响其他逻辑
	//#define HI_AUDIO_PLAY_SAVE_FILE_ON	

	#define HI_AVIO_VIDEO_ON
	/*************************************************************/
	//#define HI_VIDEO_VENC_SAVE_FILE_ON		//视频文件存储,会关闭其他逻辑
	/*************************************************************/
	#define	HI_VIDEO_GROUP_CROP			//-offline 视频裁剪属性，必须小于HI_VIDEO_GROUP_SIZE
	/*************************************************************/
	#define HI_VI_CHNNL_1_ON		//启用通道1,只能放大
	//#define HI_VI_CHNNL_2_ON		//启用通道2,只能缩小
	//#define HI_VI_CHNNL_3_ON		//启用通道3
	

#endif

	/*************************************************************/
	#define USE_SIMPLE_THREAD		
	

////////////////////////////////////////////////////////////////////////

	
	
	
	
#endif


