/*************************************************************************
	> File Name: 
	> Author: lroyd
	> Mail: htzhangxmu@163.com
	> Created Time: 
 ************************************************************************/
#ifndef __CONFIG_VI_CHNNL_H__
#define __CONFIG_VI_CHNNL_H__

#include "config_site.h"


#define __VI_SET_CHNL(a,b,c,d,e,f)      {a,b,c,d,e,f} 	

typedef struct _tagVideoChnnlInfo
{
	unsigned char m_u8Chnnl;
	unsigned char m_u8BlkCnt;				//默认是都是4，需要根据mmz动态调整
	#define VIDEO_PIX_D1				(4)
	#define VIDEO_PIX_QVGA				(6)	//320 * 240
	#define VIDEO_PIX_VGA				(7)	//640 * 480
	#define VIDEO_PIX_XGA				(8) //1024 * 768
	#define VIDEO_PIX_HD720				(16)//1280 * 720
	#define VIDEO_PIX_HD1080			(17)//1920 * 1080		
	unsigned char m_u8PixSize;			
	
	#define VIDEO_VENC_FMT_H264			(96)
	#define VIDEO_VENC_FMT_H265			(265)
	#define VIDEO_VENC_FMT_MJPEG		(1002)		
	unsigned char m_u8PayLoad;	
	
	#define VIDEO_H264_PRO_BASELINE	(0)
	#define VIDEO_H264_PRO_MP		(1)
	#define VIDEO_H264_PRO_HP		(2)		
	unsigned char m_u8Profile;	
	
	#define VIDEO_COMPRESS_MODE_NONE	(0)
	#define VIDEO_COMPRESS_MODE_SEG		(1)
	#define VIDEO_COMPRESS_MODE_SEG128	(2)
	unsigned char m_u8Compress;
}T_VideoChnnlInfo;

#define HI_VIDEO_GROUP_SIZE			(VIDEO_PIX_HD720)	//采集头大小

#ifdef HI_VIDEO_GROUP_CROP
	#define HI_VIDEO_CROP_RECT_X	(320)//绝对x起始坐标（左上原点）
	#define HI_VIDEO_CROP_RECT_Y	(120)//绝对y起始坐标（左上原点）
	#define HI_VIDEO_CROP_RECT_W	(640)//需要裁剪的宽
	#define HI_VIDEO_CROP_RECT_H	(480)//需要裁剪的高
#endif


#ifdef HI_VI_CHNNL_1_ON
	const T_VideoChnnlInfo ctUseChnnl_1 = __VI_SET_CHNL(1, 4,VIDEO_PIX_HD720, VIDEO_VENC_FMT_H264, VIDEO_H264_PRO_HP, VIDEO_COMPRESS_MODE_SEG);
#endif


#ifdef HI_VI_CHNNL_2_ON
	const T_VideoChnnlInfo ctUseChnnl_2 = __VI_SET_CHNL(2, 4,VIDEO_PIX_VGA, VIDEO_VENC_FMT_H264, VIDEO_H264_PRO_HP, VIDEO_COMPRESS_MODE_SEG);
#endif


#ifdef HI_VI_CHNNL_3_ON
	const T_VideoChnnlInfo ctUseChnnl_3 = __VI_SET_CHNL(3, 4,VIDEO_PIX_QVGA, VIDEO_VENC_FMT_H264, VIDEO_H264_PRO_HP, VIDEO_COMPRESS_MODE_NONE);
#endif




#endif


