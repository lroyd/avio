#ifndef __MODULE_H__
#define __MODULE_H__



enum
{
    NF_MOD_PRIORITY_TRANSPORT_LAYER	= 8,
    NF_MOD_PRIORITY_TSX_LAYER		= 16,
    NF_MOD_PRIORITY_UA_PROXY_LAYER	= 32,
    NF_MOD_PRIORITY_DIALOG_USAGE	= 48,
    NF_MOD_PRIORITY_APPLICATION		= 64
};

typedef struct _tagString
{
	char *pName;
	int u32Len;
}NF_STR_TYPE

typedef struct T_ModuleInfo
{
    NF_STR_TYPE name;

    int id;

    int m_emPriority;

	void *userData;				//私有数据
	
    int (*pLoad)(void *);		//NULL，直接注册成功	

    int (*pStart)(void *);

    int (*pStop)(void);

    int (*pUnload)(void);

#if 0
    BOOLEN (*on_rx_request)(void *rdata);

    BOOLEN (*on_rx_response)(void *rdata);

    int (*on_tx_request)(void *tdata);

    int (*on_tx_response)(void *tdata);

    void (*on_tsx_state)(void *tsx, void *event);
#endif
}T_ModuleInfo;







#endif 

