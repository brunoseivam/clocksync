#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "list.h"

static int list_num_elem_t(struct list *list)
{
	if(list == NULL){
		printf("list_numelem_t received a null pointer\n");
		return 0;
	}

	return list->num_elem;
}

static void list_print_node_t(struct listnode *node){
	if(node == NULL){
		printf("print_node received a null pointer\n");
		return;
	}

	printf("time.tv_sec = %ld\ntime.tv_usec = %ld\ntime.ip = %4s\n",
		node->data.time.tv_sec, node->data.time.tv_usec, node->data.ip);
}

static void list_print_all_t(struct list *list){
	struct listnode *it;

	if(list == NULL){
		printf("print_list received a null pointer\n");
		return;
	}

	if(list->list == NULL){
		printf("List was not initialized properly\n");
		return;
	}

	/* Get first valid element. Skip header */
	it = list->list->next;

	printf("num_elem = %d\n", list_num_elem_t(list));
	while(it != NULL){
		list_print_node_t(it);
		it = it->next;
	}
}

static void list_add_t(struct list *list, struct nodedata *node)
{
	struct listnode *node_i;
	struct listnode *it;

	if(list == NULL || node == NULL){
		printf("list_add_t received a null pointer\n");
		return;
	}

	node_i = (struct listnode *)malloc(sizeof(struct listnode));
	it = list->list;

	/* Copy contents of node to node_i */
	memcpy(&node_i->data.time.tv_sec, &node->time.tv_sec,
			sizeof(long));
	memcpy(&node_i->data.time.tv_usec, &node->time.tv_usec,
			sizeof(long));
	/* Ugly... */
	memcpy(node_i->data.ip, node->ip, 4*sizeof(unsigned char));

	while(it->next != NULL){
		it = it->next;
	}

	it->next = node_i;
	node_i->prev = it;

	/* Increase the number of elements in the list */
	list->num_elem++;
}

static struct listnode *list_delfirst_t(struct list *list)
{
	struct listnode *it;

	if(list == NULL){
		printf("list_del_first_t received a null pointer\n");
		return NULL;
	}

	/* No header */
	if(list->list == NULL){
		/*printf("List was not initialized properly\n");*/
		return NULL;
	}

	/* Element to be deleted */
	it = list->list->next;

	/* No elements in the list */
	if(it == NULL)
		return NULL;

	/* Pointer dance... */
	list->list->next = it->next;

	/* Test for last element in the list*/
	if(it->next != NULL)
		it->next->prev = list->list;

	list->num_elem--;
	return it;
}

static void list_delall_t(struct list *list)
{
	struct listnode *node;

	/*if(list == NULL){
		printf("list_delall_t received a null pointer\n");
		return;
	}*/

	printf("beleza ate aqui\n");
	/* First element to be deleted */
	while((node = list_delfirst_t(list)) != NULL){
		free(node);
	}

	/* Delete the header if it exists*/
	if(list->list != NULL){
		free(list->list);
		list->list = NULL;
	}
}

static int list_isempty_t(struct list *list)
{
	if(list == NULL){
		printf("list_isempty_t received a null pointer\n");
		return 0;
	}

	return (list->num_elem == 0) ? 1 : 0;
}

/* Move to application */
static const struct list_operations list_operations_t = {
	/*.list_init = &list_init_t,*/
	.list_add = &list_add_t,
	.list_delfirst = &list_delfirst_t,
	.list_delall = &list_delall_t,
	.list_isempty = &list_isempty_t,
	.list_num_elem = &list_num_elem_t,
	.list_print_node = &list_print_node_t,
	.list_print_all = &list_print_all_t,
};

int list_init(struct list *list)
{
	/*if(list_op == NULL){
		printf("list_operations received a null pointer\n");
		return -1;
	}*/

	/* Hardcoded for now */
	list->list_op = &list_operations_t;

	/* Create list element */
	list->list = (struct listnode *)malloc(sizeof(struct listnode));

	list->list->prev = list->list->next = NULL;
	/*list->list->data.time.tv_sec = list->list->data.time.tv_usec = 0;*/
	timerclear(&list->list->data.time);
	memcpy(list->list->data.ip, "AAAA", 4*sizeof(unsigned char));
	list->num_elem = 0;

	return 0;
}
