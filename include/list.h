#ifndef _LIST_H_
#define _LIST_H_

#include <sys/time.h>

struct time_listnode{
	struct time_listnode *next;
	struct time_listnode *prev;
	struct timeval time;
	unsigned char ip[4];
};

struct time_list{
	struct time_listnode *list;
	int numelem;
};

void list_init(struct time_list *list);
void list_add(struct time_list *list, struct time_listnode *node);
struct time_listnode *list_delfirst(struct time_list *list);
void list_delall(struct time_list *list);
int list_isempty(struct time_list *list);

#endif
