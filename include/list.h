#ifndef _LIST_H_
#define _LIST_H_

#include <sys/time.h>

/*void list_init(struct time_list *list);
void list_add(struct time_list *list, struct time_nodedata *node);
struct time_listnode *list_delfirst(struct time_list *list);
void list_delall(struct time_list *list);
int list_isempty(struct time_list *list);
int list_numelem(struct time_list *list);*/

/* Insert this structure inside your listnode */
#define LIST_NODE_P(type)		\
	type *next;						\
	type *prev

struct nodedata{
	struct timeval time;
	unsigned char ip[4];
};
 
/* Move to application */
struct listnode{
	LIST_NODE_P(struct listnode);
	struct nodedata data;
};

/* Foward declaration */
struct list;

struct list_operations{
	/*void (*list_init)(struct list *list);*/
	void (*list_add)(struct list *list, struct nodedata *node);
	struct listnode *(*list_delfirst)(struct list *list);
	void (*list_delall)(struct list *list);
	int (*list_isempty)(struct list *list);
	int (*list_num_elem)(struct list *list);
	void (*list_print_node)(struct listnode *node);
	void (*list_print_all)(struct list *list);
};

struct list{
	struct listnode *list;
	int num_elem;

	const struct list_operations *list_op;
};

int list_init(struct list *list);

#endif
