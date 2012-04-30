#include "adjtime.h"
#include "list.h"
#include "common.h"
#include "connection.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include <sys/time.h>


static struct list time_list;
static struct timeval send_time;

int adjust_master_prepare(void)
{
	gettimeofday(&send_time, NULL);

	/* Initialize list */
	list_init(&time_list);

	return 0;
}

int adjust_master_addslave(const unsigned char ip[4], const struct timeval *time)
{
	struct nodedata *node;
	struct timeval recv_time, rtt, slave_recv_time;

	gettimeofday(&recv_time, NULL);			// When we received slave's answer

	node = (struct nodedata*) malloc(sizeof(struct nodedata));

	/* Estimate half RTT */
	timersub(&recv_time, &send_time, &rtt);
	rtt.tv_usec /= 2;

	/* Slave received CLOCKREQ at... */
	timeradd(&send_time, &rtt, &slave_recv_time);

	/* SLAVE - MASTER clock = difference.
	 * If difference is negative, slave clock is behind master clock.
	 */
	timersub(time, &slave_recv_time, &node->time);

	/*for(i = 0; i < 4; ++i)
		node->ip[i] = ip[i];*/
	memcpy(node->ip, ip, 4*sizeof(unsigned char));

	time_list.list_op->list_add(&time_list, node);

	return 0;
}

int adjust_master_calcandsend(void)
{
	struct listnode *p;
	struct timeval sum;

	struct timeval current_time, new_time;


	if(time_list.list_op->list_isempty(&time_list))
		return -1;

	/* Sum all differences */
	timerclear(&sum);

	for(p = time_list.list; p != NULL; p = p->next)	// Head of list
	{
		struct timeval newsum;
		timeradd(&sum, &p->data.time, &newsum);

		sum.tv_sec = newsum.tv_sec;
		sum.tv_usec = newsum.tv_usec;
	}

	/* Mean of the diferences (include master (+1)) */
	sum.tv_sec /= time_list.list_op->list_num_elem(&time_list) + 1;
	sum.tv_usec /= time_list.list_op->list_num_elem(&time_list) + 1;

	/* Adjust slave clocks */
	for(p = time_list.list; p != NULL; p = p->next)
	{
		struct packet pkt;

		/* Mean difference - Slave difference */
		timersub(&sum, &p->data.time, &pkt.time);

		pkt.type    = CMD_CLOCKSYNC;
		pkt.version = CLOCKSYNC_VERSION;
		pkt.seqnum  = 0;

		send_msg(p->data.ip, &pkt);
	}

	/* Adjust master clock */
	gettimeofday(&current_time, NULL);
	timeradd(&current_time, &sum, &new_time);
	settimeofday(&new_time, NULL);

	/* Delete list */
	time_list.list_op->list_delall(&time_list);

	return 0;
}

int adjust_slave_clock(const struct timeval *diff)
{
	struct timeval current_time, new_time;

	gettimeofday(&current_time, NULL);
	timeradd(&current_time, diff, &new_time);
	settimeofday(&new_time, NULL);

	return 0;

}

