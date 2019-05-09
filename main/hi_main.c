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

#define CONTEXT_SZ (360)
typedef struct _tag  
{  
    int written;//作为一个标志，非0：表示可读，0表示可写  
    int packet_type;
	int packet_len;
	unsigned char video_road;
    unsigned char text[CONTEXT_SZ];//记录写入和读取的文本  
}shared_use_st;

shared_use_st *cap = NULL;	//写入
shared_use_st *play = NULL;	//读出	


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
	
	while(cap->written == 1) //0可写，不可读
	{
		usleep(100);
	}

	memset(cap->text, 0, len);  //海思采集必须是320
	memcpy(cap->text, p_data, len);
	cap->packet_len = len;			
	cap->written = 1;	
	
	return 0;
}

int audio_play(char *p_data, int len)
{
	play->written = 0;
	while(play->written == 0)
	{
		usleep(100);
	}

	memcpy(p_data, play->text, play->packet_len); //320
	
	return 0;
}

int video_data(int type, char *p_data, int len, unsigned long long pts)
{

	printf("1111 nal type %d, len %d\r\n", type, len);

	
	return 0;
}

int video_data2(int type, char *p_data, int len, unsigned long long pts)
{

	printf("2222 nal type %d, len %d\r\n", type, len);

	while(1)
	{
		sleep(1);
	}
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
	
	
	IPC_ShareMemory(NULL, 12345, sizeof(shared_use_st), &cap);	
	IPC_ShareMemory(NULL, 12345, sizeof(shared_use_st), &play);	
	

	signal(SIGINT, HI_AVIO_SignalHandle);
	signal(SIGTERM, HI_AVIO_SignalHandle);	
	
	ret = HI_AVIO_Init();
	printf("init %d\n", ret);
	
	ret = HI_AVIO_AudioSStart(audio_cap, audio_play);

#if 1	
	HI_AVIO_VideoSStartChannel(1, NULL);
	HI_AVIO_VideoSStartChannel(2, NULL);
	HI_AVIO_VideoSStartChannel(3, NULL);
#else	

	HI_AVIO_VideoSStartChannel(1, video_data3);
	//HI_AVIO_VideoSStartChannel(3, video_data3);
	//ret = HI_AVIO_VideoSStartChannel(3, video_data3);
	//ret = HI_AVIO_VideoSStartChannel(1, video_data2);
	//printf("audio start %d\n", ret);
#endif	
	while(1)
	{
		sleep(1);
	}
	return 0;
}


