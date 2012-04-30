#include "adjtime.h"
#include "list.h"
#include "common.h"
#include "connection.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include <sys/time.h>


static struct time_list list;
static struct timeval send_time;

int adjust_master_prepare(void)
{
	gettimeofday(&send_time, NULL);

	list_delall(&list);

	return 0;
}

int adjust_master_addslave(const unsigned char ip[4], const struct timeval *time)
{
	int i;
	struct time_listnode *node;
	struct timeval recv_time, rtt, slave_recv_time;

	gettimeofday(&recv_time, NULL);			// When we received slave's answer

	node = (struct time_listnode*) malloc(sizeof(struct time_listnode));

	/* Estimate half RTT */
	timersub(&recv_time, &send_time, &rtt);
	rtt.tv_usec /= 2;

	/* Slave received CLOCKREQ at... */
	timeradd(&send_time, &rtt, &slave_recv_time);


	/* SLAVE - MASTER clock = difference.
	 * If difference is negative, slave clock is behind master clock.
	 */
	timersub(time, &slave_recv_time, &node->time);

	for(i = 0; i < 4; ++i)
		node->ip[i] = ip[i];

	list_add(&list, node);

	return 0;
}

int adjust_master_calcandsend(void)
{
	struct time_listnode *p;
	struct timeval sum;

	struct timeval current_time, new_time;


	if(list_isempty(&list))
		return -1;

	/* Sum all differences */
	timerclear(&sum);

	for(p = list.list; p != NULL; p = p->next)	// Head of list
	{
		struct timeval newsum;
		timeradd(&sum, &p->time, &newsum);

		sum.tv_sec = newsum.tv_sec;
		sum.tv_usec = newsum.tv_usec;
	}

	/* Mean of the diferences (include master (+1)) */
	sum.tv_sec /= list.numelem + 1;
	sum.tv_usec /= list.numelem + 1;

	/* Adjust slave clocks */
	for(p = list.list; p != NULL; p = p->next)
	{
		struct packet pkt;

		/* Mean difference - Slave difference */
		timersub(&sum, &p->time, &pkt.time);

		pkt.type    = CMD_CLOCKSYNC;
		pkt.version = CLOCKSYNC_VERSION;
		pkt.seqnum  = 0;

		send_msg(p->ip, &pkt);
	}

	/* Adjust master clock */
	gettimeofday(&current_time, NULL);
	timeradd(&current_time, &sum, &new_time);
	settimeofday(&new_time, NULL);

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

