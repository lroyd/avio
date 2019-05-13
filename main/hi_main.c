/*************************************************************************
	> File Name: 
	> Author: lroyd
	> Mail: 
	> Created Time: 
 ************************************************************************/
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h> 
#include <fcntl.h>
#include <signal.h>
#include <stdarg.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include "avio_api.h"
#include "iniparser.h"
#include "rtsp_demo.h"

//rtsp
static int rtsp_enable = 0;
static int rtsp_video_chnnl = 0;	//1+
static char *rtsp_url = NULL;
rtsp_demo_handle g_rtsplive = NULL;
rtsp_session_handle session= NULL;

//rtmp
static int rtmp_enable = 0;
static int rtmp_video_chnnl = 0;	//1+
static char *rtmp_url = NULL;


int audio_cap(char *p_data, int len)
{

	return 0;
}

int audio_play(char *p_data, int len)
{

	return 0;
}


static int cancel = 0;

int user_cancel(void *p_arg)
{
	*((int *)p_arg) = 1;
	//int data = *((int *)p_arg);
	//printf("exec user_cancel %d\n", data);
	
	
	
	return 0;
}

int video_data2(int chnnl, int type, char *p_data, int len, unsigned long long pts)
{
	
	//printf("2222 nal type %d, len %d\r\n", type, len);
#if 0
	while(1)
	{
		if (cancel)
		{
			//printf("quit video_data2 %d\n",cancel);
			break;
		}
		sleep(1);
		//printf("cancel %d, addr %x\n", cancel, &cancel);
	}
	


	while(video_smm->written == 1)
	{
		if (cancel)
		{
			//printf("quit video_data2 %d\n",cancel);
			break;
		}		
		usleep(100);
	}
		

	switch(type)
	{
		case nal_unit_type_h264_sps:
			video_smm->packet_type = nal_unit_type_h264_sps;
		break;

		case nal_unit_type_h264_pps:
			video_smm->packet_type = nal_unit_type_h264_pps;
		break;

		case nal_unit_type_h264_idr:
			video_smm->packet_type = nal_unit_type_h264_idr;
		break;					

		case nal_unit_type_h264_p:
			video_smm->packet_type = nal_unit_type_h264_p;
		break;

		default:
			return 0;
	}

	memset(video_smm->text, 0, CONTEXT_SZ);
	memcpy(video_smm->text, p_data, len);
	video_smm->packet_len = len;
	
	video_smm->written = 1;
#endif
	return 0;
}



int rtsp_callbck(int chnnl, int type, char *p_data, int len, unsigned long long pts)
{
	//printf("rtsp nal type %d, len %d\r\n", type, len);
	rtsp_sever_tx_video(g_rtsplive, session, p_data, len, pts);
	
	return 0;
}


#define INI_PATH	("test.ini")

char *ini_path = NULL;

static void usage(void)
{
	printf("help? NO NO NO\n");
}


char *os_strdup(const char *s)
{
	size_t len;
	char *d;
	len = strlen(s);
	d = malloc(len + 1);
	if (d == NULL)
		return NULL;
	memcpy(d, s, len);
	d[len] = '\0';
	return d;
}

static const char *pid_file = NULL;




int parse_config(const char *path)
{
	dictionary *conf;
	//ini文件是否存在
	if (access(path, F_OK))   
    {
        fprintf(stderr,"can not find [%s] !!!\n", path);
        exit(EXIT_FAILURE);
    }	
	
    conf = iniparser_load(path);//parser the file
    if(conf == NULL)
    {
        fprintf(stderr,"can not open [%s] !!!\n", path);
        exit(EXIT_FAILURE);
    }

	//rtsp
	int channel = 0;
	char *str = NULL;
	//是否需要改成单个变量覆盖？
	if (rtsp_url == NULL && rtsp_video_chnnl == 0)
	{
		//没有配置过
		channel = iniparser_getint(conf, "rtsp:video_channel", 0);
		str = iniparser_getstring(conf, "rtsp:url", NULL);
		if (str && channel > 0)
		{
			rtsp_video_chnnl = channel;
			rtsp_url = os_strdup(str);
			rtsp_enable = 1;
		}		
	}

	if (rtmp_url == NULL && rtmp_video_chnnl == 0)
	{
		//没有配置过
		channel = iniparser_getint(conf, "rtmp:video_channel", 0);
		str = iniparser_getstring(conf, "rtmp:url", NULL);
		if (str && channel > 0)
		{
			rtmp_video_chnnl = channel;
			rtmp_url = os_strdup(str);
			rtmp_enable = 1;
		}			
	}

	

	return 0;
}

int main(int argc, char *argv[])
{
	int c, ret = -1;
	
	for (;;) 
	{
		c = getopt(argc, argv, "c:Bg:G:hi:p:P:s:v");
		if (c < 0)
			break;
		switch (c) 
		{
			case 'c':
				//配置文件
				ini_path = os_strdup(optarg);
				break;
			case 'B':
				printf("%c:%s\n", c, optarg);
				break;
			case 'g':
				printf("%c:%s\n", c, optarg);
				break;
			case 'G':
				printf("%c:%s\n", c, optarg);
				break;
			case 'h':
				usage();
				return 0;
			case 'v':
				printf("%c:%s\n", c, optarg);
				break;
			case 'i':
				printf("%c:%s\n", c, optarg);
				break;
			case 'p':
				//pid file
				//printf("%c:%s\n", c, optarg);
				pid_file = optarg;
				break;
			case 'P':
				printf("%c:%s\n", c, optarg);
				break;
			case 's':
				printf("%c:%s\n", c, optarg);
				break;				
			default:
				usage();
				return -1;
		}
	}	
	if (ini_path)
	{
		parse_config(ini_path);
	}
	

	
	
	signal(SIGINT, HI_AVIO_SignalHandle);
	signal(SIGTERM, HI_AVIO_SignalHandle);	
	
	
	ret = HI_AVIO_Init(ini_path);	
	printf("init %d\n", ret);

	ret = HI_AVIO_AudioSStart(audio_cap, audio_play);
	
	/////////////////////////////////////////////////////////////////////
	//初始化rtsp
	if (rtsp_enable)
	{
		g_rtsplive = create_rtsp_demo(554);
		session = create_rtsp_session(g_rtsplive, "/live.sdp");		
		
		HI_AVIO_VideoRegisterServer(rtsp_video_chnnl, "rtsp", rtsp_callbck, NULL, NULL);	
	}	
	

	
	
	//HI_AVIO_VideoRegisterServer(1, "rtmp", rtsp_callbck, NULL, NULL);		//rtsp测试
	//HI_AVIO_VideoRegisterServer(3, "usr2", video_data2, user_cancel, (void *)&cancel);	//取消点测试
	
	HI_AVIO_VideoStartChannel(1);
	HI_AVIO_VideoStartChannel(2);
	HI_AVIO_VideoStartChannel(3);


	while(1)
	{
		sleep(1);
	}
	return 0;
}


