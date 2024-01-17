#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <errno.h>
#include <netinet/tcp.h>
#include <netinet/ip.h>
#include <ctype.h>
#include <unistd.h>
#include <getopt.h>
#include <netinet/udp.h>
#include <netinet/ip.h>
#include <time.h>
#include <arpa/inet.h>
#include <errno.h>
#define MAX_MESSAGE 4096
#define DEFAULT_PORT 179
extern int errno;
enum msg_type {
	open = 1,
	update = 2,
	notification = 3,
	keepalive = 4,
	route_refresh = 5
};
struct header {
        uint8_t marker[16];
        uint16_t len;
        uint8_t type;
} __attribute__((packed));
struct open_message {
	uint8_t ver;
	uint16_t my_as;
	uint16_t hold_time;
	uint32_t bgp_id;
	uint8_t optional_params_len;

	uint8_t option_param_type;
	uint8_t option_param_len;

	uint8_t rr_cap_code;
	uint8_t rr_cap_len;

	uint8_t err_cap_code;
	uint8_t err_cap_len;

	uint8_t as4_as_code;
	uint8_t as4_as_len;
	uint16_t as4_as_local_as_1;
	uint16_t as4_as_local_as_2;

	uint8_t mp_cap_code;
	uint8_t mp_cap_param_len;
	uint16_t mp_cap_mp_extensions_afi;
	uint8_t mp_cap_mp_extensions_reserved;
	uint8_t mp_cap_mp_extensions_safi;

	uint8_t gr_cap_code;
	uint8_t gr_cap_len;
	uint16_t gr_cap_timers;

} __attribute__((packed));
struct keepalive_message {
	uint8_t marker[16];
	uint16_t len;
	uint8_t type;
} __attribute__((packed));

struct eor_message {
        uint8_t marker[16];
        uint16_t len;
        uint8_t type;
        uint16_t withdrawn_route_len;
	uint16_t total_pa_len;
} __attribute__((packed));



void print_usage() {
	printf("Usage: dpt <PEER_ADDRESS> <LOCAL_ASN> -s SOURCE_ADDRESS -p PEER_ASN -c CYCLES -m MSS\n");
}
int get_message_type(int sockfd, struct header *hdr) {
	
	if(recv(sockfd, hdr, sizeof(hdr), MSG_PEEK) < 0) {
		printf("Recv failed\n");
		exit(EXIT_FAILURE);
	};
	printf("Message type received: %d\n", hdr->type);
	printf("Message len received: %d\n", htons(hdr->len));
	return hdr->type;
}
int main(int argc, char *argv[]) {
	uint16_t cycles = 1;
	int mss = 1460;
	uint32_t local_asn;
	char* destination;
	int option = 0;
	while((option = getopt(argc, argv, "p:c:m:h:")) != -1) {
		switch(option) {
			break;
			case 'c':
			cycles = atoi(optarg);
			break;
			case 'm':
			mss = atoi(optarg);
			break;
			case 'h':
			print_usage();
			exit(EXIT_SUCCESS);
			default: print_usage();
				 exit(EXIT_FAILURE);
		}
	}
	if(argc < 2) {
		print_usage();
		exit(EXIT_FAILURE);
	}
	destination = argv[argc - 2];
	local_asn = atoi(argv[argc - 1]);

	char send_buf[MAX_MESSAGE];
	char recv_buf[MAX_MESSAGE];
	int sockfd;
	struct sockaddr_in peer;
	bzero((char *) &peer, sizeof(peer));

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(setsockopt(sockfd, IPPROTO_TCP, TCP_MAXSEG, &mss, sizeof(mss)) < 0) {
		printf("Failed to set socket options: %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	};
	bzero(send_buf,sizeof(send_buf));
	peer.sin_family = AF_INET;
	peer.sin_addr.s_addr = inet_addr(destination);
	peer.sin_port = htons(DEFAULT_PORT);        

        struct keepalive_message *keepalive_msg = (struct keepalive_message *)malloc(sizeof(struct keepalive_message));
        struct eor_message *eor_msg = (struct eor_message *)malloc(sizeof(struct eor_message));

        keepalive_msg->type = 4;
        keepalive_msg->len = htons(sizeof(struct keepalive_message));

        eor_msg->type = 2;
        eor_msg->len = htons(sizeof(struct eor_message));
        eor_msg->withdrawn_route_len = 0;
        eor_msg->total_pa_len = 0;

	char *datagram;
        datagram = malloc(MAX_MESSAGE);
        memset(datagram,0,MAX_MESSAGE);
	struct header *hdr = (struct header *) datagram;
	struct open_message *open_msg = (struct open_message *) (datagram + sizeof(struct header));
	hdr->type = 1;
        for(int i = 0; i < 16; i++) {
                hdr->marker[i] = 255;
                keepalive_msg->marker[i] = 255;
                eor_msg->marker[i] = 255;
        };

        open_msg->ver = 4;
        open_msg->my_as = htons(local_asn);
        open_msg->hold_time = htons(180);
        open_msg->bgp_id = 33686018;
        open_msg->optional_params_len = 22;

        open_msg->option_param_type = 2;
        open_msg->option_param_len = 20;

        open_msg->rr_cap_code = 2;
        open_msg->rr_cap_len = 0;

        open_msg->err_cap_code = 70;
        open_msg->err_cap_len = 0;

        open_msg->as4_as_code = 65;
        open_msg->as4_as_len = 4;
        open_msg->as4_as_local_as_1 = 0;
        open_msg->as4_as_local_as_2 = htons(local_asn);

        open_msg->mp_cap_code = 1;
        open_msg->mp_cap_param_len = 4;
        open_msg->mp_cap_mp_extensions_afi = htons(1);
        open_msg->mp_cap_mp_extensions_reserved = 0;
        open_msg->mp_cap_mp_extensions_safi = 1;

        open_msg->gr_cap_code = 64;
        open_msg->gr_cap_len = 2;
        open_msg->gr_cap_timers = htons(16684);
		
	int open_msg_len = htons(sizeof(struct open_message) + sizeof(struct header));
	hdr->len = open_msg_len;
	printf("Attempting to connect to %s with a local ASN of %d and a MSS of %d for %d cycles\n",destination,local_asn,mss,cycles); 

	if(connect(sockfd, (struct sockaddr *)&peer, sizeof(peer)) < 0) {	
		printf("Failed to connect: %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	};
	printf("Sending open message of size %d\n", ntohs(open_msg_len));
	int sent_bytes = (send(sockfd, datagram, open_msg_len, 0));

	if(sent_bytes < 0) {
		printf("Failed to send: %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	};

	int recv_bytes = 0;
	int type = get_message_type(sockfd,hdr);
	printf("Received message type: %d\n",type);
	recv_bytes = (recv(sockfd, recv_buf, sizeof(recv_buf), 0));
	printf("Received %d bytes from peer\n",recv_bytes);	
	sent_bytes = (send(sockfd, keepalive_msg, sizeof(struct keepalive_message), 0));
        sent_bytes = (send(sockfd, eor_msg, sizeof(struct eor_message), 0));
	
	recv_bytes = (recv(sockfd, recv_buf, sizeof(recv_buf), 0));
	sleep(30);
	
	printf("Actually sent %d number of bytes\n",sent_bytes);
	close(sockfd);
	exit(EXIT_SUCCESS);
}
