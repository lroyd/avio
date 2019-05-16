#include <stdio.h>
#include <string.h>




//////////////////////////////////////////////////////////////////////////////////
typedef struct dl_list 
{
	struct dl_list *next;
	struct dl_list *prev;
}dl_list;

#define DL_LIST_HEAD_INIT(l) { &(l), &(l) }

static inline void dl_list_init(struct dl_list *list)
{
	list->next = list;
	list->prev = list;
}

static inline void dl_list_add(struct dl_list *list, struct dl_list *item)
{
	item->next = list->next;
	item->prev = list;
	list->next->prev = item;
	list->next = item;
}

static inline void dl_list_add_tail(struct dl_list *list, struct dl_list *item)
{
	dl_list_add(list->prev, item);
}

static inline void dl_list_del(struct dl_list *item)
{
	item->next->prev = item->prev;
	item->prev->next = item->next;
	item->next = NULL;
	item->prev = NULL;
}

static inline int dl_list_empty(struct dl_list *list)
{
	return list->next == list;
}

static inline unsigned int dl_list_len(struct dl_list *list)
{
	struct dl_list *item;
	int count = 0;
	for (item = list->next; item != list; item = item->next)
		count++;
	return count;
}

#ifndef offsetof
#define offsetof(type, member) ((long) &((type *) 0)->member)
#endif

#define dl_list_entry(item, type, member) \
	((type *) ((char *) item - offsetof(type, member)))

#define dl_list_first(list, type, member) \
	(dl_list_empty((list)) ? NULL : \
	 dl_list_entry((list)->next, type, member))

#define dl_list_last(list, type, member) \
	(dl_list_empty((list)) ? NULL : \
	 dl_list_entry((list)->prev, type, member))

#define dl_list_for_each(item, list, type, member) \
	for (item = dl_list_entry((list)->next, type, member); \
	     &item->member != (list); \
	     item = dl_list_entry(item->member.next, type, member))

#define dl_list_for_each_safe(item, n, list, type, member) \
	for (item = dl_list_entry((list)->next, type, member), \
		     n = dl_list_entry(item->member.next, type, member); \
	     &item->member != (list); \
	     item = n, n = dl_list_entry(n->member.next, type, member))

#define dl_list_for_each_reverse(item, list, type, member) \
	for (item = dl_list_entry((list)->prev, type, member); \
	     &item->member != (list); \
	     item = dl_list_entry(item->member.prev, type, member))

#define DEFINE_DL_LIST(name) \
	struct dl_list name = { &(name), &(name) }


//////////////////////////////////////////////////////////////////////////////////
typedef struct _tagData
{
	char    value;
    char    p; 
    dl_list list1;      //tail
    dl_list list2;
    

    
}T_Data;

typedef struct _tag
{
	char    v;
    dl_list list;
   
    
}T_Node;

T_Node *g_table_node[15] = {NULL};

T_Data g_data;


int list_test(void)
{
    int i = 10;
    
    
    dl_list_init(&g_data.list1);
    dl_list_init(&g_data.list2);
    
    for(i=0;i<10;i++)
    {
        g_table_node[i] = (T_Node *)malloc(sizeof(T_Node));
		memset(g_table_node[i], 0, sizeof(T_Node));
        //dl_list_init(&g_table_node[i]->list);
        g_table_node[i]->v = i;
        
        dl_list_add_tail(&g_data.list1, &g_table_node[i]->list);
        //dl_list_add(&g_data.list2, &g_table_node[i]->list);     //头加入
    }
    
	int len;
	
	
    T_Node *p = NULL, *n = NULL;

	len = dl_list_len(&g_data.list1);
	printf("===================list1 = %d save ==============================\n", len);
	dl_list_for_each_safe(p, n, &g_data.list1, T_Node, list)
    {
        printf("get val = %d\n", p->v);
		dl_list_del(&p->list);
		free(p);
    }	
	
	len = dl_list_len(&g_data.list1);
    printf("===================list1 = %d ==============================\n", len);
	
    dl_list_for_each(p, &g_data.list1, T_Node, list)    //从后面遍历？
    {
        printf("get val = %d\n", p->v);
		//dl_list_del(&p->list);
    }	

	
	len = dl_list_len(&g_data.list2);
    printf("===================list2 = %d ==============================\n",  len);
    p = NULL;
     dl_list_for_each(p, &g_data.list2, T_Node, list)    //从后面遍历？
    {
        printf("get val = %d\n", p->v);
    }   
    

}
