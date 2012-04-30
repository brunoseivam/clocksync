#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <sys/time.h>
#include <string.h>

#include "connection.h"
#include "common.h"

#define PORT "3700"

enum state
{
   STATE_STARTUP			= 1 << 5,
   STATE_SLAVE				= 2 << 5,
   STATE_SLAVE_ADJTIME	= 3 << 5,
   STATE_CANDIDATE		= 4 << 5,
   STATE_ACCEPT			= 5 << 5,
   STATE_MASTER			= 6 << 5,
   STATE_MASTER_ADJTIME	= 7 << 5
};

const unsigned char BROADCAST[4] = {255,255,255,255};
/*const unsigned char BROADCAST[4] = {192,168,1,255};*/
const struct timeval STARTUP_TIMEOUT =
{
	.tv_sec = 5,
	.tv_usec = 0
};

const struct timeval SLAVE_TIMEOUT =
{
	.tv_sec = 5,
	.tv_usec = 0
};

const struct timeval SLAVE_ADJTIME_TIMEOUT =
{
	.tv_sec = 0,
	.tv_usec = 100000
};

const struct timeval CANDIDATE_TIMEOUT =
{
	.tv_sec = 5,
	.tv_usec = 0
};

const struct timeval MASTER_TIMEOUT =
{
	.tv_sec = 4,
	.tv_usec = 0
};

const struct timeval MASTER_ADJTIME_TIMEOUT =
{
	.tv_sec = 0,
	.tv_usec = 100000
};

const struct timeval ACCEPT_TIMEOUT =
{
	.tv_sec = 5,
	.tv_usec = 0
};

void set_timer(struct timeval* tval, struct timeval timeout)
{
	tval->tv_sec = timeout.tv_sec;
	tval->tv_usec = timeout.tv_usec;
}

void build_send_packet(const unsigned char dest[4],
		                 unsigned char cmd, int put_time)
{
	struct packet pkt;
	pkt.type = cmd;
	pkt.version = 1;
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
		            struct timeval *tval, struct timeval timeout)
{
	*current_state = next_state;
   set_timer(tval, timeout);

	switch(next_state)
	{
		case STATE_STARTUP:
			printf("STARTUP\n");
			break;
		case STATE_SLAVE:
			printf("SLAVE\n");
			break;
		case STATE_SLAVE_ADJTIME:
			printf("SLAVE_ADJTIME\n");
			break;
		case STATE_CANDIDATE:
			printf("CANDIDATE\n");
			break;
		case STATE_ACCEPT:
			printf("ACCEPT\n");
			break;
		case STATE_MASTER:
			printf("MASTER\n");
			break;
		case STATE_MASTER_ADJTIME:
			printf("MASTER_ADJTIME\n");
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

	/* Startup: */
	build_send_packet(BROADCAST, CMD_MASTERREQ, 0); /* Search for a master */
	change_state(&state, STATE_STARTUP, &tval, STARTUP_TIMEOUT);

   for(;;)
   {
      recv_msg(&pkt, &tval);

		command = pkt.type;
		printf("Command received = %d\n", command);

      switch(state | command)
      {
			/*    STARTUP    */
         case(STATE_STARTUP | CMD_TIMEOUT):
				build_send_packet(BROADCAST, CMD_MASTERUP, 0);
            change_state(&state, STATE_MASTER, &tval, MASTER_TIMEOUT);
            break;

			case(STATE_STARTUP | CMD_MASTERUP):
				build_send_packet(pkt.ip, CMD_SLAVEUP, 0);
				// no break;
			case(STATE_STARTUP | CMD_MASTERREQ):
			case(STATE_STARTUP | CMD_MASTERACK):
			case(STATE_STARTUP | CMD_ELECTION):
				change_state(&state, STATE_SLAVE, &tval, SLAVE_TIMEOUT);
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
				change_state(&state, STATE_MASTER_ADJTIME, &tval, MASTER_ADJTIME_TIMEOUT);
				break;

			case(STATE_MASTER | CMD_MASTERREQ):
				build_send_packet(pkt.ip, CMD_MASTERACK, 0);
				break;

			case(STATE_MASTER | CMD_QUIT):
				build_send_packet(pkt.ip, CMD_ACK, 0);
				change_state(&state, STATE_SLAVE, &tval, SLAVE_TIMEOUT);
				break;

			case(STATE_MASTER | CMD_ELECTION):
			case(STATE_MASTER | CMD_MASTERUP):
				build_send_packet(pkt.ip, CMD_QUIT, 0);
				break;

         /*    MASTER_ADJTIME    */
			case(STATE_MASTER_ADJTIME | CMD_TIMEOUT):
				/* Calculate avg clocks and send to each slave his correction */
				/* Restart the synchronization timer */
				change_state(&state, STATE_MASTER, &tval, MASTER_TIMEOUT);
				break;

         /*    SLAVE    */
         case(STATE_SLAVE | CMD_CLOCKREQ):
            printf("got clock req!\n");
				/* Send clock packet to master and change to an intermediate state
				 * in order to wait for a synch packet */
				/*build_send_packet(pkt.ip, CMD_QUIT, 0);*/
            change_state(&state, STATE_SLAVE_ADJTIME, &tval, SLAVE_ADJTIME_TIMEOUT);
            break;

			case(STATE_SLAVE | CMD_TIMEOUT):
				build_send_packet(BROADCAST, CMD_ELECTION, 0);
				change_state(&state, STATE_CANDIDATE, &tval, CANDIDATE_TIMEOUT);
            break;

			case(STATE_SLAVE | CMD_MASTERUP):
				build_send_packet(BROADCAST, CMD_SLAVEUP, 0);
				break;

			case(STATE_SLAVE | CMD_ELECTION):
				build_send_packet(pkt.ip, CMD_ACCEPT, 0);
				change_state(&state, STATE_ACCEPT, &tval, ACCEPT_TIMEOUT);
				break;

         /*    SLAVE ADJTIME   */
			case(STATE_SLAVE_ADJTIME | CMD_CLOCKSYNC):
				/* Receive packet from master, adjust local time and return to
				 * your rightful state (slave of course... =])*/
				change_state(&state, STATE_SLAVE, &tval, SLAVE_TIMEOUT);
				break;

				/* Merge this case with the above */
			/* Slave sent his clock to master but did not receive his synch packet */
			/* What can I do? */
			case(STATE_SLAVE_ADJTIME | CMD_TIMEOUT):
				change_state(&state, STATE_SLAVE, &tval, SLAVE_TIMEOUT);
				break;

			/*    CANDIDATE   */
			case(STATE_CANDIDATE | CMD_TIMEOUT):
				build_send_packet(BROADCAST, CMD_MASTERUP, 0);
				change_state(&state, STATE_MASTER, &tval, MASTER_TIMEOUT);
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
				change_state(&state, STATE_SLAVE, &tval, SLAVE_TIMEOUT);
				break;

			/*    ACCEPT    */
			case(STATE_ACCEPT | CMD_TIMEOUT):
				change_state(&state, STATE_SLAVE, &tval, SLAVE_TIMEOUT);
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

