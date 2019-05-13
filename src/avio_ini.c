/*******************************************************************************
	> File Name: 
	> Author: lroyd
	> Mail: htzhangxmu@163.com
	> Created Time: 
 *******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "iniparser.h"
#include "config_site.h"

enum{
    AUDIO_ENABLE = 0,
    AUDIO_EC,
	AUDIO_LOOKUP,
	
	AUDIO_AI_SAVE,
	AUDIO_AO_SAVE,
	
	AUDIO_AI_VOLUME,
	AUDIO_AO_VOLUME,
	
	AUDIO_LIST_MAX
};

const char *audio_list[AUDIO_LIST_MAX] = {
	"audio:enable",
	"audio:ec",
	"audio:lookup",
	
	"audio:ai_save",
	"audio:ao_save",
	
	"audio:ai_volume",
	"audio:ao_volume",
	
};

enum{
    VIDEO_ENABLE = 0,
	VIDEO_SAVE,
	VIDEO_SIZE,
	VIDEO_CORP,
	
	VIDEO_CHNNL_1,
	VIDEO_CHNNL_2,
	VIDEO_CHNNL_3,
	
	VIDEO_LIST_MAX
};

const char *video_list[VIDEO_LIST_MAX] = {
	"video:enable",
	"video:save_file",
	"video:size",
	"video:corp",
	
	"video:channel_1",
	"video:channel_2",
	"video:channel_3",
	
};


//(320,120):(640,480)格式
static int parseCrop(const char *_pString, int *_u32X, int *_u32Y, int *_u32W, int *_u32H)
{
	int ret = -1;
	char *start = NULL, *end = NULL, str[8] = {0};

	start = strchr(_pString, '(');
	if (!start) goto EXIT;	
	
	end = strchr(start, ',');
	if (!end) goto EXIT;
	
	//x
	memcpy(str, start + 1, end - start - 1);
	//printf("=====x %s\n", str);
	*_u32X = atoi(str);
	//y
	start = end + 1;
	end = strchr(start, ')');
	if (!end) goto EXIT;
	
	memset(str , 0, 8);
	memcpy(str, start, end - start);
	//printf("=====y %s\n", str);
	*_u32Y = atoi(str);
	//w
	start = end + 3;
	end = strchr(start, ',');
	if (!end) goto EXIT;
	
	memset(str , 0, 8);
	memcpy(str, start, end - start);	
	//printf("=====w %s\n", str);	
	*_u32W = atoi(str);
	
	//h
	start = end + 1;
	end = strchr(start, ')');
	if (!end) goto EXIT;
	
	memset(str , 0, 8);
	memcpy(str, start, end - start);		
	//printf("=====h %s\n", str);
	*_u32H = atoi(str);	

	ret = 0;
EXIT:	
	return ret;
}

//(1, 4, 16, 96, 2, 1)
static int parseChannel(const char *_pString, T_VideoChnnlInfo **_pVideoChnnl)
{
	int ret = -1;
	T_VideoChnnlInfo *pVideoChnnl = NULL;
	unsigned char u8Chnnl = 0, u8BlkCnt = 0, u8PixSize = 0, u8PayLoad = 0, u8Profile = 0, u8Compress = 0;
	char *start = NULL, *end = NULL, str[8] = {0};
	
	//u8Chnnl
	start = strchr(_pString, '(');
	if (!start) goto EXIT;		
	end = strchr(start, ',');
	if (!end) goto EXIT;
	memset(str , 0, 8);
	memcpy(str, start + 1, end - start - 1);
	u8Chnnl = atoi(str);	
	//printf("xxxxxx %d, %s\n", u8Chnnl, str);
	//u8BlkCnt
	start = end + 1;	
	end = strchr(start, ',');
	if (!end) goto EXIT;	
	memset(str , 0, 8);
	memcpy(str, start, end - start);
	u8BlkCnt = atoi(str);	
	
	//u8PixSize
	start = end + 1;	
	end = strchr(start, ',');
	if (!end) goto EXIT;	
	memset(str , 0, 8);
	memcpy(str, start, end - start);
	u8PixSize = atoi(str);	
	
	//u8PayLoad
	start = end + 1;	
	end = strchr(start, ',');
	if (!end) goto EXIT;	
	memset(str , 0, 8);
	memcpy(str, start, end - start);
	u8PayLoad = atoi(str);		
	
	//u8Profile
	start = end + 1;	
	end = strchr(start, ',');
	if (!end) goto EXIT;	
	memset(str , 0, 8);
	memcpy(str, start, end - start);
	u8Profile = atoi(str);		
	
	//u8Compress
	start = end + 1;	
	end = strchr(start, ')');
	if (!end) goto EXIT;	
	memset(str , 0, 8);
	memcpy(str, start, end - start);
	u8Compress = atoi(str);	
	
	pVideoChnnl = (T_VideoChnnlInfo *)malloc(sizeof(T_VideoChnnlInfo));
	pVideoChnnl->m_u8Chnnl		= u8Chnnl;
	pVideoChnnl->m_u8BlkCnt		= u8BlkCnt;
	pVideoChnnl->m_u8PixSize	= u8PixSize;
	pVideoChnnl->m_u8PayLoad	= u8PayLoad;
	pVideoChnnl->m_u8Profile	= u8Profile;
	pVideoChnnl->m_u8Compress	= u8Compress;
	
	ret = 0;
EXIT:	
	*_pVideoChnnl = pVideoChnnl;
	return ret;
}

int iniAvioParseConfig(const char *_pConfigPath, T_AudioConfigInfo *_pAudio, T_VideoConfigInfo *_pVideo, T_VideoChnnlInfo **_pVideoChnnlTable)
{
	dictionary *conf;
	//ini文件是否存在
	if (access(_pConfigPath, F_OK))   
    {
        fprintf(stderr,"can not find [%s] !!!\n", _pConfigPath);
        exit(EXIT_FAILURE);
    }
	
    conf = iniparser_load(_pConfigPath);//parser the file
    if(conf == NULL)
    {
        fprintf(stderr,"can not open [%s] !!!\n", _pConfigPath);
        exit(EXIT_FAILURE);
    }
	
	int enbale = -1, i;
	//获取音频参数
	enbale = iniparser_getint(conf, audio_list[AUDIO_ENABLE], 0);
	if (enbale)
	{
		printf("[audio]\n");
		_pAudio->m_bEnable = 1;
		_pAudio->m_bEC = iniparser_getint(conf, audio_list[AUDIO_EC], 0);
		_pAudio->m_bLookup = iniparser_getint(conf, audio_list[AUDIO_LOOKUP], 0);
		
		_pAudio->m_bAiSave = iniparser_getint(conf, audio_list[AUDIO_AI_SAVE], 0);
		_pAudio->m_bAoSave = iniparser_getint(conf, audio_list[AUDIO_AO_SAVE], 0);
		
		_pAudio->m_s32AiVolume = iniparser_getint(conf, audio_list[AUDIO_AI_VOLUME], 0);
		_pAudio->m_s32AoVolume = iniparser_getint(conf, audio_list[AUDIO_AO_VOLUME], 0);		
		
		printf("set enable = %d, ec = %d, lookup = %d, ai_save = %d, ao_save = %d, ai_volume = %d, ao_volume = %d\n", 
				_pAudio->m_bEnable, _pAudio->m_bEC, _pAudio->m_bLookup, _pAudio->m_bAiSave, _pAudio->m_bAoSave, 
				_pAudio->m_s32AiVolume, _pAudio->m_s32AoVolume);
	}
	
	//获取视频参数
	enbale = iniparser_getint(conf, video_list[VIDEO_ENABLE], 0);
	if (enbale)
	{
		_pVideo->m_bEnable = 1;
		
		_pVideo->m_bSave = iniparser_getint(conf, video_list[VIDEO_SAVE], 0);
		
		_pVideo->m_s32GroupSize = iniparser_getint(conf, video_list[VIDEO_SIZE], 7);	//必须有默认VGA
		
		const char *str = NULL;
		str = iniparser_getstring(conf, video_list[VIDEO_CORP], NULL);
		if (str)
		{
			//crop
			int x,y,w,h;
			if (parseCrop(str, &x, &y, &w, &h))
			{
				_pVideo->in_tCrop.m_bEnable = 0;
				printf("parse crop parameter error\n");
			}
			else
			{
				_pVideo->in_tCrop.m_bEnable = 1;
				_pVideo->in_tCrop.m_u32X	= x;
				_pVideo->in_tCrop.m_u32Y	= y;
				_pVideo->in_tCrop.m_u32W	= w;
				_pVideo->in_tCrop.m_u32H	= h;	
				//printf("crop: x = %d, y = %d, w = %d, h = %d\n",x, y, w, h);
			}
		}
		
		//解析通道
		for (i = 0; i < HI_VIDEO_CHNNL_NUM; i++)
		{
			str = iniparser_getstring(conf, video_list[VIDEO_CHNNL_1 + i], NULL);
			if (str)
			{
				//printf("=========== %d, %s\n", i , str);
				T_VideoChnnlInfo *pVideoChnnl = NULL;
				if (parseChannel(str, &pVideoChnnl))
				{
					printf("parse video channel [%d] parameter error\n", i + 1);
				}
				else
				{
					printf("config channel[%d]: %d, %d, %d, %d, %d\n", pVideoChnnl->m_u8Chnnl, pVideoChnnl->m_u8BlkCnt, pVideoChnnl->m_u8PixSize, 
																		pVideoChnnl->m_u8PayLoad, pVideoChnnl->m_u8Profile, pVideoChnnl->m_u8Compress);
				}
				*(_pVideoChnnlTable + i) = pVideoChnnl;
			}
		}
		
		
		
	}	
	
	return 0;
}


