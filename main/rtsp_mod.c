/*******************************************************************************
	> File Name: 
	> Author: lroyd
	> Mail: htzhangxmu@163.com
	> Created Time: 
 *******************************************************************************/
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>

#include "rtsp_mod.h"
#include "rtsp_demo.h"


rtsp_demo_handle g_rtsplive = NULL;
rtsp_session_handle session= NULL;

static int rtspLoad(void *_pArg)
{
	
	g_rtsplive = create_rtsp_demo(554);
	session = create_rtsp_session(g_rtsplive, "/live.sdp");			
	
	return 0;
}


static int rtspStart(void *_pArg)
{
	
	rtsp_sever_tx_video(g_rtsplive, session, p_data, len, pts);
	
	return 0;
}

static int rtspStop(void *_pArg)
{
	
	
	
	return 0;
}

static int rtspUnload(void *_pArg)
{
	
	
	
	return 0;
}



T_ModuleInfo rtsp_module_handle = 
{
    { "mod-rtsp-handler", 16},		/* name.		*/
    -1,								/* id			*/
    NF_MOD_PRIORITY_APPLICATION,	/* priority	        */
	NULL,							/* usr data 	*/
    rtspLoad,						/* load()		*/
    rtspStart,						/* start()		*/
    rtspStop,						/* stop()		*/
    rtspUnload,						/* unload()		*/
	
#if 0	
    &default_mod_on_rx_request,		/* on_rx_request()	*/
    NULL,							/* on_rx_response()	*/
    NULL,							/* on_tx_request.	*/
    NULL,							/* on_tx_response()	*/
    NULL,							/* on_tsx_state()	*/
#endif
};


