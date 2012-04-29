#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "connection.h"
#include "pack.h"
#include "common.h"

#define PORT "3700"

static int sockfd;
//static struct addrinfo *own_addr;

const char* cmd_str(enum command cmd)
{
	switch(cmd)
	{
		case CMD_MASTERREQ:
			return "MASTER REQUEST";
		case CMD_MASTERACK:
			return "MASTER ACKNOWLEDGE";
		case CMD_ELECTION:
			return "ELECTION";
		case CMD_MASTERUP:
			return "MASTER UP";
		case CMD_SLAVEUP:
			return "SLAVE UP";
		case CMD_ACCEPT:
			return "ACCEPT CANDIDATURE";
		case CMD_REFUSE:
			return "REFUSE CANDIDATURE";
		case CMD_QUIT:
			return "QUIT";
		case CMD_TIMEOUT:
			return "TIMED OUT WAITING COMMAND";
		case CMD_ACK:
			return "ACK";
	}
	return "UNKNOWN COMMAND";

}

void print_packet(struct packet *pkt){
	printf("packet: type: %2X %s\n", pkt->type, cmd_str(pkt->type));
/*	printf("packet: version: %2X\n", pkt->version);
	printf("packet: seqnum: %d\n", pkt->seqnum);
	printf("packet: time: %us %us\n", (unsigned int) pkt->time.tv_sec, (unsigned int) pkt->time.tv_usec);
	printf("packet: ip: %.4s\n", pkt->ip);*/
}

void print_msg(char *src_addr, struct packet *pkt, struct timeval *tval){
	printf("src addr: %d.%d.%d.%d\n", (unsigned char)src_addr[0], (unsigned char)src_addr[1],
		(unsigned char)src_addr[2], (unsigned char)src_addr[3]);
	print_packet(pkt);
	printf("timeleft: %ds %dus\n", (int) tval->tv_sec, (int) tval->tv_usec);
}

void *get_in_addr(struct sockaddr *sa){
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

/* There is no way to use this function "as is" on the same host.
	They ...*/
int open_udp_socket(){
	struct addrinfo hints, *servinfo, *p;
	int rv, optval = 1;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC; //AF_INET for IP4 only
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_PASSIVE; // use my IP

	if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return -1;
	}

	// loop through all the results and bind to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
						p->ai_protocol)) == -1) {
			perror("socket");
			continue;
		}

		/* Set socket to reuse address. Workaround for abnormal clousures */
		if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval,
					sizeof(optval)) == -1){
			perror("setsockopt");
			return -1;
		}

		/* Set socket to brodcast */
		if(setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &optval,
					sizeof(optval)) == -1){
			printf("dadsa\n");
			perror("setsockopt");
			return -1;
		}

		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("bind");
			continue;
		}

		break;
	}

	if (p == NULL) {
		fprintf(stderr, "failed to bind socket\n");
		return -1;
	}

	freeaddrinfo(servinfo);

	return 0;
}

int send_msg(const unsigned char dest_addr_ip[4], const struct packet *pkt){
	struct addrinfo hints, *servinfo;
	int rv, numbytes;
	unsigned char buf[BUFLEN];
	//uint32_t *timevalp;

	char dest_addr[16];  // xxx.xxx.xxx.xxx\0

	sprintf(dest_addr, "%d.%d.%d.%d", (unsigned int) dest_addr_ip[0],
			                            (unsigned int) dest_addr_ip[1],
												 (unsigned int) dest_addr_ip[2],
												 (unsigned int) dest_addr_ip[3]);

	printf("SEND TO %s\n", dest_addr);

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC; //AF_INET;
	hints.ai_socktype = SOCK_DGRAM;

	if ((rv = getaddrinfo(dest_addr, PORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return -1;
	}

	if(!servinfo){
		fprintf(stderr, "destination not found %s\n", dest_addr);
		return -1;
	}

	/* Serialize structure */
	/* FIXME Only works for the same endianess. timeval structure */
	pack(buf, "cchzzcccc", (int8_t)pkt->type, (int8_t)pkt->version, (int16_t)pkt->seqnum,
			(int64_t)pkt->time.tv_sec, (int64_t)pkt->time.tv_usec, (int8_t)pkt->ip[0],
			(int8_t)pkt->ip[1], (int8_t)pkt->ip[2], (int8_t)pkt->ip[3]);

		/* Send UDP packet */
		if((numbytes = sendto(sockfd, buf, PACKETSIZE, 0,
						servinfo->ai_addr, servinfo->ai_addrlen)) == -1){
			perror("sendto");
			return -1;
		}

	return numbytes;
}

int recv_msg(struct packet *pkt, struct timeval *tv){
	socklen_t their_addr_len;
	struct sockaddr_storage their_addr;
	int numbytes;
	unsigned char buf[BUFLEN];
	fd_set readfds;

	FD_ZERO(&readfds);
	FD_SET(sockfd, &readfds);

	select(sockfd+1, &readfds, NULL, NULL, tv);

	/* Timeout expired */
	if(!FD_ISSET(sockfd, &readfds)){
		pkt->type = CMD_TIMEOUT;
	}
	else{ /* Received message */
		their_addr_len = sizeof(their_addr);

		if((numbytes = recvfrom(sockfd, buf, PACKETSIZE, 0,
						(struct sockaddr *)&their_addr, &their_addr_len)) == -1){
			perror("rcvfrom");
			return -1;
		}

		/* Deserialize structure */
		/* FIXME Only works for the same endianess. timeval structure */
		unpack(buf, "cchzzcccc", &pkt->type, &pkt->version, &pkt->seqnum,
				&pkt->time.tv_sec, &pkt->time.tv_usec, &pkt->ip[0], &pkt->ip[1],
				&pkt->ip[2], &pkt->ip[3]);

		/* Get sorce ip. Working only for IPv4 for now */
		*((int *)pkt->ip) = *((int *)get_in_addr((struct sockaddr *)&their_addr));
	}
	return 0;
}
