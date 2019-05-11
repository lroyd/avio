/*******************************************************************************
	> File Name: 
	> Author: lroyd
	> Mail: htzhangxmu@163.com
	> Created Time: 
 *******************************************************************************/
#include <stdio.h>
#include <unistd.h>

#include "list.h"

typedef struct _tagListNodeStd
{
    _DECL_LIST_MEMBER(struct _tagListNodeStd);
	void *pArg;	//参数组
} T_ListNode;

typedef struct _tagServerNode
{
    _DECL_LIST_MEMBER(struct _tagServerNode);
	const char		*m_pName;	//目前没有

    HI_VIDEO_CBK	m_pHandle;
	HI_VIDEO_CANCEL m_pCancel;		//每一个server都应该有一个取消点
	void			*pCancelArg;	//取消点参数		
} T_ServerNode;

typedef struct _tagViServer
{
	T_ServerNode	in_tList;		//服务链表头
	int				m_u32Cnt;		//当前服务个数
}T_ViServerInfo;


//获得所有节点个数
int LIST_NodeGetTotal(void *_pList)
{
	T_ListNode *pList = (T_ListNode *)_pList;
	return NF_ListGetTotal(pList);
}

//删除所有节点
int LIST_NodeCleanAll(void *_pList)
{
	T_ListNode *pList = (T_ListNode *)_pList;
	int u32Num = 0;
	
	
	
	return u32Num;
}

//将节点加入链表
int LIST_NodeAdd(void *_pList, void *_pNode, int _u32Len)
{
	T_ListNode *pList = (T_ListNode *)_pList;
	T_ListNode *pNode = (T_ListNode *)_pNode;
	char *p = (char *)malloc(_u32Len);
	if (!p) 
	{
		return -1;
	}
	
	memcpy(p, _pNode, _u32Len);
	
	NF_ListInsertAppend(pList, pNode);
	
	return 0;
}

//按位置删除？按节点删除
int LIST_NodeDelete(void *_pList)
{
	
	
	return 0;
}

//遍历每一个节点,返回Node，
int LIST_NodeForeach(void *_pList, void **_pNode, unsigned char *_u8Pos)
{
	unsigned char u8Pos = *_u8Pos;
	
	
	return 0;
}

//取出每个节点特定值
int LIST_NodeForeachGetValue(void *_pList)
{
	return 0;
}

//取出特定节点对应的特定值
int LIST_NodeFindGetValue(void *_pList)
{
	return 0;
}







