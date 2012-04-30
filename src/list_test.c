#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "list.h"

int main()
{
	struct list my_list;
	struct nodedata my_nodedata;

	list_init(&my_list);
	printf("----------------\n");
	my_list.list_op->list_print_node(my_list.list);
	printf("----------------\n");

	my_nodedata.time.tv_sec = 1;
	my_nodedata.time.tv_usec = 1;
	memcpy(my_nodedata.ip, "BBBB", 4*sizeof(unsigned char));
	my_list.list_op->list_add(&my_list, &my_nodedata);

	my_nodedata.time.tv_sec = 2;
	my_nodedata.time.tv_usec = 2;
	memcpy(my_nodedata.ip, "CCCC", 4*sizeof(unsigned char));
	my_list.list_op->list_add(&my_list, &my_nodedata);

	printf("----------------\n");
	my_list.list_op->list_print_all(&my_list);
	printf("----------------\n");

	my_list.list_op->list_delfirst(&my_list);

	printf("----------------\n");
	my_list.list_op->list_print_all(&my_list);
	printf("----------------\n");

	my_list.list_op->list_delall(&my_list);

	printf("----------------\n");
	my_list.list_op->list_print_all(&my_list);
	printf("----------------\n");

	list_init(&my_list);

	printf("----------------\n");
	my_list.list_op->list_print_all(&my_list);
	printf("----------------\n");

	return 0;
}
