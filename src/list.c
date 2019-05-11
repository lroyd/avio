/*******************************************************************************
	> File Name: 
	> Author: lroyd
	> Mail: htzhangxmu@163.com
	> Created Time: 
 *******************************************************************************/
#include <stdio.h>
#include <unistd.h>

#include "list.h"


void NF_ListInit(NF_LIST_TYPE *node)
{
    ((T_NFList*)node)->next = ((T_NFList*)node)->prev = node;
}


int NF_ListEmpty(const NF_LIST_TYPE * node)
{
    return ((T_NFList*)node)->next == node;
}


static void NF_LinkNode(NF_LIST_TYPE *prev, NF_LIST_TYPE *next)
{
    ((T_NFList*)prev)->next = next;
    ((T_NFList*)next)->prev = prev;
}

void NF_ListInsertAppend(NF_LIST_TYPE *pos, NF_LIST_TYPE *node)
{
    NF_ListInsertPreter(((T_NFList*)pos)->prev, node);
}


void NF_ListPushBack(NF_LIST_TYPE *list, NF_LIST_TYPE *node)
{
    NF_ListInsertAppend(list, node);
}


void NF_ListInsterNodesAppend(NF_LIST_TYPE *pos, NF_LIST_TYPE *lst)
{
    NF_ListInsterNodesAfter(((T_NFList*)pos)->prev, lst);
}


void NF_ListInsertPreter(NF_LIST_TYPE *pos, NF_LIST_TYPE *node)
{
    ((T_NFList*)node)->prev = pos;
    ((T_NFList*)node)->next = ((T_NFList*)pos)->next;
    ((T_NFList*) ((T_NFList*)pos)->next) ->prev = node;
    ((T_NFList*)pos)->next = node;
}


void NF_ListPushFront(NF_LIST_TYPE *list, NF_LIST_TYPE *node)
{
    NF_ListInsertPreter(list, node);
}



void NF_ListInsterNodesAfter(NF_LIST_TYPE *pos, NF_LIST_TYPE *lst)
{
    T_NFList *lst_last = (T_NFList *) ((T_NFList*)lst)->prev;
    T_NFList *pos_next = (T_NFList *) ((T_NFList*)pos)->next;

    NF_LinkNode(pos, lst);
    NF_LinkNode(lst_last, pos_next);
}


void NF_ListMergeFirst(NF_LIST_TYPE *lst1, NF_LIST_TYPE *lst2)
{
    if (!NF_ListEmpty(lst2))
	{
		NF_LinkNode(((T_NFList*)lst2)->prev, ((T_NFList*)lst1)->next);
		NF_LinkNode(((T_NFList*)lst1), ((T_NFList*)lst2)->next);
		NF_ListInit(lst2);
    }
}



void NF_ListMergeLast(NF_LIST_TYPE *lst1, NF_LIST_TYPE *lst2)
{
    if (!NF_ListEmpty(lst2)) 
	{
		NF_LinkNode(((T_NFList*)lst1)->prev, ((T_NFList*)lst2)->next);
		NF_LinkNode(((T_NFList*)lst2)->prev, lst1);
		NF_ListInit(lst2);
    }
}



void NF_ListNodeDelete(NF_LIST_TYPE *node)
{
    NF_LinkNode(((T_NFList*)node)->prev, ((T_NFList*)node)->next);
    NF_ListInit(node);
}


NF_LIST_TYPE* NF_ListFindNode(NF_LIST_TYPE *list, NF_LIST_TYPE *node)
{
    T_NFList *p = (T_NFList *) ((T_NFList*)list)->next;
    while (p != list && p != node)
	{
		p = (T_NFList *) p->next;
	}
	
    return p==node ? p : NULL;
}


NF_LIST_TYPE* NF_ListSearch(NF_LIST_TYPE *list, void *value, int (*comp)(void *value, const NF_LIST_TYPE *node))
{
    T_NFList *p = (T_NFList *) ((T_NFList*)list)->next;
    while (p != list && (*comp)(value, p) != 0)
	p = (T_NFList *) p->next;

    return p==list ? NULL : p;
}


int NF_ListGetTotal(const NF_LIST_TYPE *list)
{
    const T_NFList *node = (const T_NFList*) ((const T_NFList*)list)->next;
    int count = 0;

    while (node != list) 
	{
		++count;
		node = (T_NFList*)node->next;
    }

    return count;
}


