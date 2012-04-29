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
   STATE_STARTUP		= 1 << 5,
   STATE_SLAVE			= 2 << 5,
   STATE_CANDIDATE	= 3 << 5,
   STATE_ACCEPT		= 4 << 5,
   STATE_MASTER		= 5 << 5
};


const unsigned char BROADCAST[4] = {255,255,255,255};
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

const struct timeval CANDIDATE_TIMEOUT =
{
	.tv_sec = 5,
	.tv_usec = 0
};

const struct timeval MASTER_TIMEOUT =
{
	.tv_sec = 5,
	.tv_usec = 0
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
		case STATE_CANDIDATE:
			printf("CANDIDATE\n");
			break;
		case STATE_ACCEPT:
			printf("ACCEPT\n");
			break;
		case STATE_MASTER:
			printf("MASTER\n");
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

		printf("------------------\n");
		printf("got packet:\n");
		print_packet(&pkt);
		printf("------------------\n");

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
			case(STATE_MASTER | CMD_TIMEOUT):
				change_state(&state, STATE_MASTER, &tval, MASTER_TIMEOUT);
				// ADJUST TIMES
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

         /*    SLAVE    */
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

