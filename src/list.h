#ifndef __ULIST_H__
#define __ULIST_H__

#ifdef __cplusplus
extern "C" {
#endif



#define _DECL_LIST_MEMBER(type)                       \
                                   type *prev;          \
                                   type *next 


typedef struct _tagString
{
    char	*ptBuf;
    int		m_s32Len;
}NF_STR_TYPE;

typedef	void NF_LIST_TYPE;

typedef struct _tagList
{
    _DECL_LIST_MEMBER(void);
}T_NFList; /* may_alias avoids warning with gcc-4.4 -Wall -O2 */

//初始化节点 next = prev = node
void NF_ListInit(NF_LIST_TYPE *node);

//检查当前节点的next，ture空，false非空
int NF_ListEmpty(const NF_LIST_TYPE * node);


//void NF_LinkNode(NF_LIST_TYPE *prev, NF_LIST_TYPE *next);

//表尾追加(可以是节点)
//list	->n1
//		->n1	->n2
//		->n1	->n2	->n3
void NF_ListInsertAppend(NF_LIST_TYPE *pos, NF_LIST_TYPE *node);
//表头前插(可以是节点)
//list					->n1
//				->n2	->n1
//		->n3	->n2	->n1
void NF_ListInsertPreter(NF_LIST_TYPE *pos, NF_LIST_TYPE *node);

//指定节点后追加
void NF_ListInsterNodesAppend(NF_LIST_TYPE *pos, NF_LIST_TYPE *lst);
void NF_ListInsterNodesAfter(NF_LIST_TYPE *pos, NF_LIST_TYPE *lst);

void NF_ListPushBack(NF_LIST_TYPE *list, NF_LIST_TYPE *node);
void NF_ListPushFront(NF_LIST_TYPE *list, NF_LIST_TYPE *node);



//将第一个表元素合并到第二个表中
void NF_ListMergeFirst(NF_LIST_TYPE *lst1, NF_LIST_TYPE *lst2);
//将第二个表元素合并到第一个表中
void NF_ListMergeLast(NF_LIST_TYPE *lst1, NF_LIST_TYPE *lst2);

//删除某个节点
void NF_ListNodeDelete(NF_LIST_TYPE *node);

//查找某个节点
NF_LIST_TYPE* NF_ListFindNode(NF_LIST_TYPE *list, NF_LIST_TYPE *node);

//根据特定规则搜索对应值的节点
NF_LIST_TYPE* NF_ListSearch(NF_LIST_TYPE *list, void *value, int (*comp)(void *value, const NF_LIST_TYPE *node));

//获得当前节点后续总个数
int NF_ListGetTotal(const NF_LIST_TYPE *list);


#ifdef __cplusplus
}
#endif

#endif	

