#ifndef _CONNECTION_H_
#define _CONNECTION_H_

/* Should suffice for this application */
#define BUFLEN 64
#define PACKETSIZE 24

/* Main strucute */
struct packet{
	unsigned char type;
	unsigned char version;
	unsigned short seqnum;
	struct timeval time;
	unsigned char ip[4];
};

/* Open UDP broadcast socket */
int open_udp_socket(void);
int send_msg(const unsigned char dest_addr_ip[4], const struct packet *pkt);
int recv_msg(struct packet *pkt, struct timeval *tv);

void print_packet(struct packet *pkt);
void print_msg(char* src_addr, struct packet *pkt, struct timeval *tval);
#endif
