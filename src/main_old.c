#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <sys/time.h>
#include <string.h>

#include "connection.h"

#define PORT "3700"

enum state
{
   STATE_STARTUP = 32,
   STATE_SLAVE,
   STATE_CANDIDATE,
   STATE_ACCEPT,
   STATE_MASTER
};

enum command
{
   CMD_TIMEOUT,
   CMD_ACK,
   CMD_MASTERREQ,
   CMD_MASTERACK,
   CMD_MASTERUP,
   CMD_SLAVEUP,
   CMD_ELECTION,
   CMD_ACCEPT,
   CMD_REFUSE
};


unsigned char state, command;
int timeout;

void alarm_handler(int signumber)
{
   timeout = 1;
   printf("ALARM!\n");
}

int main(int argc, char **argv)
{
   /*struct sigaction signal_action;*/
   struct timeval tval;
	struct packet pkt;

	/* Open a socket and start listening to a scpefied port */
	open_udp_socket();

	if(argc < 3){
		fprintf(stderr, "Error: usage udp_test [client|server] <dest_ip>\n");
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

	/* Client */
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
	/* Server */
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

   /*tval.it_interval.tv_sec = 5;
   tval.it_interval.tv_usec = 0;

   tval.it_value.tv_sec = 5;
   tval.it_value.tv_usec = 0;

   signal_action.sa_handler = alarm_handler;
   sigemptyset(&signal_action.sa_mask);
   signal_action.sa_flags = 0;

   sigaction(SIGALRM, &signal_action, NULL);

   setitimer(ITIMER_REAL, &tval, NULL);

   // setup code:

   state = STATE_STARTUP;

   send(CMD_MASTERREQ);
   //set nomaster timer


   for(;;)
   {
      if(timeout)
      {
         command = CMD_TIMEOUT;
         timeout = 0;
      }
      else
         recv(command);

      switch(state | command)
      {
         case(STATE_SLAVE | CMD_TIMEOUT):
            send(CMD_ELECTION);
            // set candidate timer
            state = STATE_CANDIDATE;
            break;

         case(STATE_SLAVE | CMD_ADJTIME):
            adjust_time();
            break;

         case(STATE_STARTUP | CMD_TIMEOUT):
            send(CMD_MASTERUP);
            state = STATE_MASTER;
            break;

         case (STATE_STARTUP | CMD_MASTERUP):
            send(SLAVE_UP);

         case (STATE_STARTUP | CMD_MASTERACK):
         case (STATE_STARTUP | CMD_MASTERREQ):
         case (STATE_STARTUP | CMD_ELECTION):

            state = STATE_SLAVE;
            break;

         case (STATE_SLAVE | CMD_ELECTION):

            send(CMD_ACCEPT);
            // set accept timer
            state = STATE_ACCEPT;
            break;

         case (STATE_ACCEPT | CMD_TIMEOUT):
            //set candidate timer
            state = STATE_SLAVE;
            break;

         case (STATE_ACCEPT | CMD_ELECTION):
            send(CMD_REFUSE);
            break;

         case (STATE_CANDIDATE | CMD_TIMEOUT):
            send(CMD_MASTERUP);
            state = STATE_MASTER;
            break;

         case (STATE_MASTER | CMD_TIMEOUT):
            collect_clocks();
            send_adjtimes();
            break;
      }
   }

   while(1)
   {
     printf(".");
     fflush(stdout);
     sleep(1);
   }*/


   return 0;
}
