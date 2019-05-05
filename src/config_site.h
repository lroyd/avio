/*************************************************************************
	> File Name: 
	> Author: lroyd
	> Mail: htzhangxmu@163.com
	> Created Time: 
 ************************************************************************/
#ifndef __CONFIG_AVIO_H__
#define __CONFIG_AVIO_H__


//#define HI_AVIO_AUDIO_ON
//音频参数设置
	#define	HI_AUDIO_SAMPLE_RATE		(8000)	//采样率
	#define	HI_AUDIO_BIT_WIDTH			(1)		//8:0 16:1
	#define HI_AUDIO_PTNUMPERFRM		(160)	
	#define HI_AUDIO_AO_VOLUME			(6)		//音量0-6
	#define HI_AUDIO_EC_ON						//是否开启回声抑制(默认开启)
	//#define HI_AUDIO_LOOKUP_ON					//本地回环测试(默认关闭)
	//#define HI_AUDIO_CAP_SAVE_FILE_NAME	("cap.pcm")
	//#define HI_AUDIO_PLAY_SAVE_FILE_NAME	("play.pcm")



#define HI_AVIO_VIDEO_ON
	#define VIDEO_PIX_D1				(4)
	#define VIDEO_PIX_QVGA				(6)	//320 * 240
	#define VIDEO_PIX_VGA				(7)	//640 * 480
	#define VIDEO_PIX_XGA				(8) //1024 * 768
	#define VIDEO_PIX_HD720				(16)//1280 * 720
	#define VIDEO_PIX_HD1080			(17)//1920 * 1080
	
	#define VIDEO_PIX_FMT_RGB_565				(7)
	#define VIDEO_PIX_FMT_YUV_PLANAR_422		(19)
	#define VIDEO_PIX_FMT_YUV_PLANAR_420		(20)
	#define VIDEO_PIX_FMT_YUV_PLANAR_444		(21)
	#define VIDEO_PIX_FMT_YUV_SEMIPLANAR_422	(22)
	#define VIDEO_PIX_FMT_YUV_SEMIPLANAR_420	(23)	//注意：这个SAMPLE_PIXEL_FORMAT 在ive vpss都有用
	#define VIDEO_PIX_FMT_YUV_SEMIPLANAR_444	(24)
	#define VIDEO_PIX_FMT_UYVY_PACKAGE_422		(25)
	#define VIDEO_PIX_FMT_YUYV_PACKAGE_422		(26)
	#define VIDEO_PIX_FMT_VYUY_PACKAGE_422		(27)
	#define VIDEO_PIX_FMT_YCbCr_PLANAR			(28)
	
	#define VIDEO_VENC_FMT_H264			(96)
	#define VIDEO_VENC_FMT_H265			(265)
	#define VIDEO_VENC_FMT_MJPEG		(1002)
	
	#define VIDEO_H264_PROFILE_BASELINE	(0)
	#define VIDEO_H264_PROFILE_MP		(1)
	#define VIDEO_H264_PROFILE_HP		(2)
/////////////////////////////////////////////////////////////////////
	//#define HI_VIDEO_VENC_SAVE_FILE_ON

	#define	HI_VIDEO_CHNNL_NUM			(3)	//视频通道路数 1/2/3
	
	
	
	
	
	
	
	
	

#endif


