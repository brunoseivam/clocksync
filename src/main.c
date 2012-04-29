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

void build_packet(struct packet *pkt, unsigned char cmd, int put_time)
{
	pkt->type = cmd;
	pkt->version = 1;
	pkt->seqnum = 0;

	if(put_time)
		gettimeofday(&pkt->time, NULL);
	else
	{
		pkt->time.tv_sec = 0;
		pkt->time.tv_usec = 0;
	}
}

void set_timer(struct timeval* tval, struct timeval timeout)
{
	tval->tv_sec = timeout.tv_sec;
	tval->tv_usec = timeout.tv_usec;
}


int main(int argc, char **argv)
{
	unsigned char	command, state;
   struct timeval	tval;
	struct packet	pkt;

	/* Open a socket and start listening to a scpefied port */
	open_udp_socket();

	/* Startup: */
	build_packet(&pkt, CMD_MASTERREQ, 0); /* Prepare message     */
	send_msg(BROADCAST, &pkt);			     /* Search for a master */
	printf("------------------\n");
	print_packet(&pkt);
	printf("------------------\n");
   state = STATE_STARTUP;					  /* Set STARTUP state   */
	set_timer(&tval, STARTUP_TIMEOUT);	  /* Set timeout         */
	printf("STATE: STARTUP\n");

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
				build_packet(&pkt, CMD_MASTERUP, 0);
            send_msg(BROADCAST, &pkt);
				printf("------------------\n");
				print_packet(&pkt);
				printf("------------------\n");
            state = STATE_MASTER;
				printf("STATE: MASTER\n");
				set_timer(&tval, MASTER_TIMEOUT);	  /* Set timeout         */
            break;

			case(STATE_STARTUP | CMD_MASTERUP):
				build_packet(&pkt, CMD_SLAVEUP, 0);
				send_msg(pkt.ip, &pkt);
				printf("------------------\n");
				print_packet(&pkt);
				printf("------------------\n");
				// no break;
			case(STATE_STARTUP | CMD_MASTERREQ):
			case(STATE_STARTUP | CMD_MASTERACK):
			case(STATE_STARTUP | CMD_ELECTION):
				state = STATE_SLAVE;
				printf("STATE: SLAVE\n");
				set_timer(&tval, SLAVE_TIMEOUT);
				break;

			/*    MASTER    */
			case(STATE_MASTER | CMD_TIMEOUT):
				set_timer(&tval, MASTER_TIMEOUT);
				// ADJUST TIMES
				break;

			case(STATE_MASTER | CMD_MASTERREQ):
				build_packet(&pkt, CMD_MASTERACK, 0);
				send_msg(pkt.ip, &pkt);
				printf("------------------\n");
				print_packet(&pkt);
				printf("------------------\n");
				break;

			case(STATE_MASTER | CMD_QUIT):
				build_packet(&pkt, CMD_ACK, 0);
				send_msg(pkt.ip, &pkt);
				printf("------------------\n");
				print_packet(&pkt);
				printf("------------------\n");
				state = STATE_SLAVE;
				printf("STATE_SLAVE\n");
				set_timer(&tval, SLAVE_TIMEOUT);
				break;

			case(STATE_MASTER | CMD_ELECTION):
			case(STATE_MASTER | CMD_MASTERUP):
				build_packet(&pkt, CMD_QUIT, 0);
				send_msg(pkt.ip, &pkt);
				printf("------------------\n");
				print_packet(&pkt);
				printf("------------------\n");
				break;



         /*    SLAVE    */
			case(STATE_SLAVE | CMD_TIMEOUT):
				build_packet(&pkt, CMD_ELECTION, 0);
				send_msg(BROADCAST, &pkt);
				printf("------------------\n");
				print_packet(&pkt);
				printf("------------------\n");
				state = STATE_CANDIDATE;
				printf("STATE_CANDIDATE\n");
				set_timer(&tval, CANDIDATE_TIMEOUT);
            break;

			case(STATE_SLAVE | CMD_MASTERUP):
				build_packet(&pkt, CMD_SLAVEUP, 0);
				send_msg(BROADCAST, &pkt);
				printf("------------------\n");
				print_packet(&pkt);
				printf("------------------\n");
				break;

			case(STATE_SLAVE | CMD_ELECTION):
				build_packet(&pkt, CMD_ACCEPT, 0);
				send_msg(pkt.ip, &pkt);
				printf("------------------\n");
				print_packet(&pkt);
				printf("------------------\n");
				state = STATE_ACCEPT;
				printf("STATE_ACCEPT\n");
				set_timer(&tval, ACCEPT_TIMEOUT);
				break;

			/*    CANDIDATE   */
			case(STATE_CANDIDATE | CMD_TIMEOUT):
				build_packet(&pkt, CMD_MASTERUP, 0);
				send_msg(BROADCAST, &pkt);
				printf("------------------\n");
				print_packet(&pkt);
				printf("------------------\n");
				state = STATE_MASTER;
				printf("STATE_MASTER\n");
				set_timer(&tval, MASTER_TIMEOUT);
				break;

			case(STATE_CANDIDATE | CMD_ELECTION):
				build_packet(&pkt, CMD_REFUSE, 0);
				send_msg(pkt.ip, &pkt);
				printf("------------------\n");
				print_packet(&pkt);
				printf("------------------\n");
				break;

			case(STATE_CANDIDATE | CMD_ACCEPT):
				build_packet(&pkt, CMD_ACK, 0);
				send_msg(pkt.ip, &pkt);
				printf("------------------\n");
				print_packet(&pkt);
				printf("------------------\n");
				break;

			case(STATE_CANDIDATE | CMD_QUIT):
			case(STATE_CANDIDATE | CMD_REFUSE):
				build_packet(&pkt, CMD_ACK, 0);
				send_msg(pkt.ip, &pkt);
				printf("------------------\n");
				print_packet(&pkt);
				printf("------------------\n");
				//no break

			case(STATE_CANDIDATE | CMD_MASTERREQ):
				state = STATE_SLAVE;
				printf("STATE_SLAVE\n");
				set_timer(&tval, SLAVE_TIMEOUT);
				break;

			/*    ACCEPT    */
			case(STATE_ACCEPT | CMD_TIMEOUT):
				state = STATE_SLAVE;
				printf("STATE_SLAVE\n");
				set_timer(&tval, SLAVE_TIMEOUT);
				break;

			case(STATE_ACCEPT | CMD_ELECTION):
				build_packet(&pkt, CMD_REFUSE, 0);
				send_msg(pkt.ip, &pkt);
				printf("------------------\n");
				print_packet(&pkt);
				printf("------------------\n");
				break;

			case(STATE_ACCEPT | CMD_MASTERUP):
				build_packet(&pkt, CMD_SLAVEUP, 0);
				send_msg(pkt.ip, &pkt);
				printf("------------------\n");
				print_packet(&pkt);
				printf("------------------\n");
				break;

      }
   }

   while(1)
   {
     printf(".");
     fflush(stdout);
     sleep(1);
   }


   return 0;
}
/*	printf("unsigned char size = %d\n", sizeof(unsigned char));
	printf("unsigned short size = %d\n", sizeof(unsigned short));
	printf("long int size = %d\n", sizeof(long int));
	printf("long size = %d\n", sizeof(long));
	printf("tval.tv_sec = %d\n", sizeof(tval.tv_sec));
	printf("tval.tv_usec = %d\n", sizeof(tval.tv_usec));
	printf("struct timeval size = %d\n", sizeof(struct timeval));
	printf("packet size = %d\n", sizeof(struct packet));
	printf("unsigned long= %d\n", sizeof(unsigned long));*/

	/* Client *
	if(!strcmp(argv[1], "client")){
		short count = 0;
		while(1){
			pkt.type = 0xAA;
			pkt.version = 0xBB;
			pkt.seqnum = count;
			gettimeofday(&pkt.time, NULL);
			strncpy((char*) pkt.ip, "ipip", 4);
			printf("-------------------------\n");
			print_packet(&pkt);
			printf("-------------------------\n");
			send_msg(argv[2], &pkt);
			count++;
			sleep(1);
		}
	}
	* Server *
	else{
		char src_addr[4];
		while(1){
			tval.tv_usec = 0;
			tval.tv_sec = 5;
			recv_msg(src_addr, &pkt, &tval);
			printf("-------------------------\n");
			print_msg(src_addr, &pkt, &tval);
			printf("-------------------------\n");
		}
	}

   *tval.it_interval.tv_sec = 5;
   tval.it_interval.tv_usec = 0;

   tval.it_value.tv_sec = 5;
   tval.it_value.tv_usec = 0;

   signal_action.sa_handler = alarm_handler;
   sigemptyset(&signal_action.sa_mask);
   signal_action.sa_flags = 0;

   sigaction(SIGALRM, &signal_action, NULL);

   setitimer(ITIMER_REAL, &tval, NULL);*/
