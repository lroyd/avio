/*************************************************************************
	> File Name: 
	> Author: lroyd
	> Mail: 
	> Created Time: 
 ************************************************************************/
#include <stdio.h>
#include <string.h>
#include <unistd.h>
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

#include "rtsp_demo.h"

rtsp_demo_handle g_rtsplive = NULL;
rtsp_session_handle session= NULL;

typedef enum  {
  nal_unit_type_h264_nr = 0,
  nal_unit_type_h264_p = 1,
  nal_unit_type_h264_dataA = 2,
  nal_unit_type_h264_dataB = 3,
  nal_unit_type_h264_dataC = 4,
  nal_unit_type_h264_idr = 5,
  nal_unit_type_h264_sei = 6,
  nal_unit_type_h264_sps = 7,
  nal_unit_type_h264_pps = 8,
  nal_unit_type_h264_delimiter = 9,
  nal_unit_type_h264_nalend = 10,
  nal_unit_type_h264_streamend = 11,
  nal_unit_type_h264_pading = 12
}nal_unit_type_h264;

#define CONTEXT_SZ 100*1024//(360)
typedef struct _tag  
{  
    int written;//作为一个标志，非0：表示可读，0表示可写  
	nal_unit_type_h264 packet_type;
	int packet_len;
	unsigned char video_road;
    unsigned char text[CONTEXT_SZ];//记录写入和读取的文本  
}shared_use_st;

//shared_use_st *cap = NULL;	//音频写入
//shared_use_st *play = NULL;	//音频读出	
shared_use_st *video_smm = NULL;

int IPC_ShareMemory(char *_strPath, int _s32Key, int _u32Size, void **_pOutAddr)
{
	int s32Ret = -1;
	key_t key;
	int shmid;
	if (_strPath)
	{
		if((key = ftok(_strPath, _s32Key)) < 0) 
		{
			printf("fail to ftok\n");
			goto EXIT;
		}
	}
	else
	{
		key = (key_t)_s32Key;
	}
	
	if((shmid = shmget(key, _u32Size, 0666 | IPC_CREAT)) < 0) 
	{
		printf("fail to shmget\n");
		goto EXIT;
	}
	
	if((*_pOutAddr = (void *)shmat(shmid, NULL, 0)) == (void *)-1) 
	{
		printf("fail to shmat");
		goto EXIT;
	}
	
	s32Ret = 0;
	
EXIT:	
	
	return s32Ret;
}




int audio_cap(char *p_data, int len)
{
	//HI_AVIO_AudioPlayImmt(p_data, len); //直接播放
#if 0	
	while(cap->written == 1) //0可写，不可读
	{
		usleep(100);
	}

	memset(cap->text, 0, len);  //海思采集必须是320
	memcpy(cap->text, p_data, len);
	cap->packet_len = len;			
	cap->written = 1;	
#endif	
	return 0;
}

int audio_play(char *p_data, int len)
{
#if 0	
	play->written = 0;
	while(play->written == 0)
	{
		
		usleep(100);
	}

	memcpy(p_data, play->text, play->packet_len); //320
#endif	
	return 0;
}

int video_data(int type, char *p_data, int len, unsigned long long pts)
{

	printf("1111 nal type %d, len %d\r\n", type, len);

	
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

int video_data2(int type, char *p_data, int len, unsigned long long pts)
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
	
#else

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



int video_data3(int type, char *p_data, int len, unsigned long long pts)
{
	//printf("3333 nal type %d, len %d\r\n", type, len);
	
	if(g_rtsplive)
	{
		rtsp_sever_tx_video(g_rtsplive, session, p_data, len, pts);
	}
	
	return 0;
}



int main()
{
	int ret = -1;
	
	g_rtsplive = create_rtsp_demo(554);
	session= create_rtsp_session(g_rtsplive, "/live.sdp");	
	
	//IPC_ShareMemory(NULL, 12345, sizeof(shared_use_st), &cap);	
	//IPC_ShareMemory(NULL, 12345, sizeof(shared_use_st), &play);	
	
	//IPC_ShareMemory(NULL, 88888, sizeof(shared_use_st), &video_smm);	

	signal(SIGINT, HI_AVIO_SignalHandle);
	signal(SIGTERM, HI_AVIO_SignalHandle);	
	
	ret = HI_AVIO_Init();
	printf("init %d\n", ret);
	
	ret = HI_AVIO_AudioSStart(audio_cap, audio_play);

#if 0	
	HI_AVIO_VideoSStartChannel(1, NULL);
	HI_AVIO_VideoSStartChannel(2, NULL);
	HI_AVIO_VideoSStartChannel(3, NULL);
#endif	

	HI_AVIO_VideoSStartChannel(1, video_data3);
	//HI_AVIO_VideoSStartChannel(3, video_data2);
	//HI_AVIO_VideoSetTestCancel(3, user_cancel, (void *)&cancel);
	//ret = HI_AVIO_VideoSStartChannel(3, video_data3);
	//ret = HI_AVIO_VideoSStartChannel(1, video_data2);
	//printf("audio start %d\n", ret);

	while(1)
	{
		sleep(1);
	}
	return 0;
}


