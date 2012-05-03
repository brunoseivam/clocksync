#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <sys/time.h>
#include <string.h>
#include <time.h>

#include "connection.h"
#include "common.h"
#include "adjtime.h"

#define PORT "3700"
#define TIME_SYNC 60
/* Must be greater than TIME_SYNC */
#define MIN_TIMEOUT (1.1*TIME_SYNC)
#define MAX_TIMEOUT (2*TIME_SYNC)

#define CANDIDATE_TIMEOUT 0.5*TIME_SYNC
#define MASTER_TIMEOUT TIME_SYNC
#define MASTER_ADJTIME_TIMEOUT 0.5*TIME_SYNC
#define CANDIDATE_TIMEOUT 0.5*TIME_SYNC
#define ACCEPT_TIMEOUT 0.5*TIME_SYNC

enum state
{
   STATE_STARTUP			= 1 << 5,
   STATE_SLAVE				= 2 << 5,
   STATE_CANDIDATE		= 3 << 5,
   STATE_ACCEPT			= 4 << 5,
   STATE_MASTER			= 5 << 5,
   STATE_MASTER_ADJTIME	= 6 << 5
};

const unsigned char BROADCAST[4] = {255,255,255,255};

/*struct timeval STARTUP_TIMEOUT =
{
	.tv_sec = 5,
	.tv_usec = 0
};

struct timeval SLAVE_TIMEOUT =
{
	.tv_sec = 5,
	.tv_usec = 0
};*/

const struct timeval candidate_timeout =
{
	.tv_sec = CANDIDATE_TIMEOUT,
	.tv_usec = 0
};

const struct timeval master_timeout =
{
	.tv_sec = MASTER_TIMEOUT,
	.tv_usec = 0
};

const struct timeval master_adjtime_timeout =
{
	.tv_sec = MASTER_ADJTIME_TIMEOUT,
	.tv_usec = 0
};

const struct timeval accept_timeout =
{
	.tv_sec = ACCEPT_TIMEOUT,
	.tv_usec = 0
};

void print_time(struct timeval *time)
{
	time_t now_time;
	struct tm *now_tm;
	char tm_buf[64];

	now_time = time->tv_sec;
	now_tm = localtime(&now_time);

	strftime(tm_buf, sizeof(tm_buf), "%d/%m/%Y %H:%M:%S", now_tm);
	printf("%s.%06d\n", tm_buf, (unsigned int)time->tv_usec);
}

void set_timer(struct timeval* tval, const struct timeval *timeout)
{
	/* rand timer initializer */
	if(timeout == NULL){
		tval->tv_sec = (rand() % MAX_TIMEOUT) + MIN_TIMEOUT;
		tval->tv_usec = (rand() % MAX_TIMEOUT);
		return;
	}

	tval->tv_sec = timeout->tv_sec;
	tval->tv_usec = timeout->tv_usec;
}

void build_send_packet(const unsigned char dest[4],
		                 unsigned char cmd, int put_time)
{
	struct packet pkt;
	pkt.type = cmd;
	pkt.version = CLOCKSYNC_VERSION;
	pkt.seqnum = 0;

	if(put_time)
		gettimeofday(&pkt.time, NULL);
	else
	{
		pkt.time.tv_sec = 0;
		pkt.time.tv_usec = 0;
	}

	send_msg(dest, &pkt);
}

void change_state(enum state *current_state, enum state next_state,
		            struct timeval *tval, const struct timeval *timeout)
{
	*current_state = next_state;
   set_timer(tval, timeout);

	switch(next_state)
	{
		case STATE_STARTUP:
			printf("STARTUP");
			break;
		case STATE_SLAVE:
			printf("SLAVE");
			break;
		case STATE_CANDIDATE:
			printf("CANDIDATE");
			break;
		case STATE_ACCEPT:
			printf("ACCEPT");
			break;
		case STATE_MASTER:
			printf("MASTER");
			break;
		case STATE_MASTER_ADJTIME:
			printf("MASTER_ADJTIME");
			break;
	}
}

int main(int argc, char **argv)
{
	enum state state;
	unsigned char	command;
   struct timeval	tval;
	struct packet	pkt;

	/* Open a socket and start listening to a scpefied port */
	open_udp_socket();
	srand(time(NULL));

	/* Startup: */
	build_send_packet(BROADCAST, CMD_MASTERREQ, 0); /* Search for a master */
	change_state(&state, STATE_STARTUP, &tval, NULL);

   for(;;)
   {
      struct timeval curtime;
      gettimeofday(&curtime, NULL);

		printf("  ");
		print_time(&curtime);
      /*printf(" TIME: %10d sec %10d usec\n",
				(unsigned int)curtime.tv_sec, (unsigned int)curtime.tv_usec);*/

      recv_msg(&pkt, &tval);

		command = pkt.type;
		//printf("Command received = %d\n", command);

      switch(state | command)
      {
			/*    STARTUP    */
         case(STATE_STARTUP | CMD_TIMEOUT):
				build_send_packet(BROADCAST, CMD_MASTERUP, 0);
            change_state(&state, STATE_MASTER, &tval, &master_timeout);
            break;

			case(STATE_STARTUP | CMD_MASTERUP):
				build_send_packet(pkt.ip, CMD_SLAVEUP, 0);
				// no break;
			case(STATE_STARTUP | CMD_MASTERREQ):
			case(STATE_STARTUP | CMD_MASTERACK):
			case(STATE_STARTUP | CMD_ELECTION):
				change_state(&state, STATE_SLAVE, &tval, NULL);
				break;

			/*    MASTER    */
			/* Launch a new thread and process separately? Prevents loss of messages */
			case(STATE_MASTER | CMD_TIMEOUT):
				/* Does the Master needs to send his clock? */
				build_send_packet(BROADCAST, CMD_CLOCKREQ, 1);
				//change_state(&state, STATE_MASTER, &tval, MASTER_TIMEOUT);
				/* Change to an intermediate state in order to wait for all slaves
				 * to send their clocks. After the MASTER_ADJTIME_TIMEOUT no more clock
				 * packets will be accepted and the "slow" slaves, if any, won't
				 * be synchronized*/
				change_state(&state, STATE_MASTER_ADJTIME, &tval, &master_adjtime_timeout);
				/* Possibly new thread? Non blocking function...*/
				adjust_master_prepare();
				break;

			case(STATE_MASTER | CMD_MASTERREQ):
				build_send_packet(pkt.ip, CMD_MASTERACK, 0);
				break;

			case(STATE_MASTER | CMD_QUIT):
				build_send_packet(pkt.ip, CMD_ACK, 0);
				change_state(&state, STATE_SLAVE, &tval, NULL);
				break;

			case(STATE_MASTER | CMD_ELECTION):
			case(STATE_MASTER | CMD_MASTERUP):
				build_send_packet(pkt.ip, CMD_QUIT, 0);
				break;

         /*    MASTER_ADJTIME    */
			case(STATE_MASTER_ADJTIME | CMD_CLOCKREQ_RESPONSE):
				/* Got time from client */
				adjust_master_addslave(pkt.ip, &pkt.time);
				break;

			case(STATE_MASTER_ADJTIME | CMD_TIMEOUT):
				/* Calculate avg clocks and send to each slave his correction */
				/* Restart the synchronization timer */
				change_state(&state, STATE_MASTER, &tval, &master_timeout);
				adjust_master_calcandsend();
				break;

         /*    SLAVE    */
         case(STATE_SLAVE | CMD_CLOCKREQ):
				/* Send clock packet to master and change to an intermediate state
				 * in order to wait for a synch packet */
				build_send_packet(pkt.ip, CMD_CLOCKREQ_RESPONSE, 1);
            change_state(&state, STATE_SLAVE, &tval, NULL);
            break;

			case(STATE_SLAVE | CMD_TIMEOUT):
				build_send_packet(BROADCAST, CMD_ELECTION, 0);
				change_state(&state, STATE_CANDIDATE, &tval, &candidate_timeout);
            break;

			case(STATE_SLAVE | CMD_MASTERUP):
				build_send_packet(BROADCAST, CMD_SLAVEUP, 0);
				break;

			case(STATE_SLAVE | CMD_ELECTION):
				build_send_packet(pkt.ip, CMD_ACCEPT, 0);
				change_state(&state, STATE_ACCEPT, &tval, &accept_timeout);
				break;

			case(STATE_SLAVE | CMD_CLOCKSYNC):
				/* Receive packet from master, adjust local time and return to
				 * your rightful state (slave of course... =])*/
				adjust_slave_clock(&pkt.time);
				change_state(&state, STATE_SLAVE, &tval, NULL);
				break;

			/*    CANDIDATE   */
			case(STATE_CANDIDATE | CMD_TIMEOUT):
				build_send_packet(BROADCAST, CMD_MASTERUP, 0);
				change_state(&state, STATE_MASTER, &tval, &master_timeout);
				break;

			case(STATE_CANDIDATE | CMD_ELECTION):
				build_send_packet(pkt.ip, CMD_REFUSE, 0);
				break;

			case(STATE_CANDIDATE | CMD_ACCEPT):
				build_send_packet(pkt.ip, CMD_ACK, 0);
				break;

			case(STATE_CANDIDATE | CMD_QUIT):
			case(STATE_CANDIDATE | CMD_REFUSE):
				build_send_packet(pkt.ip, CMD_ACK, 0);
				//no break

			case(STATE_CANDIDATE | CMD_MASTERREQ):
				change_state(&state, STATE_SLAVE, &tval, NULL);
				break;

			/*    ACCEPT    */
			case(STATE_ACCEPT | CMD_TIMEOUT):
				change_state(&state, STATE_SLAVE, &tval, NULL);
				break;

			case(STATE_ACCEPT | CMD_ELECTION):
				build_send_packet(pkt.ip, CMD_REFUSE, 0);
				break;

			case(STATE_ACCEPT | CMD_MASTERUP):
				build_send_packet(pkt.ip, CMD_SLAVEUP, 0);
				break;

      }
   }

   return 0;
}

