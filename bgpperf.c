#include <math.h>
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
} __attribute__((packed));

struct capabilities {
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

struct update_message {
        uint16_t withdrawn_route_len;
        uint16_t total_pa_len;
} __attribute__((packed));

struct eor_message {
        uint8_t marker[16];
        uint16_t len;
        uint8_t type;
        uint16_t withdrawn_route_len;
	uint16_t total_pa_len;
} __attribute__((packed));


void print_usage() {
	printf("Usage: bgpperf <PEER_ADDRESS> <LOCAL_ASN> -s SOURCE_ADDRESS -c CYCLES -m MSS\n");
}
float get_variance(float valarr[], float avg, int vals) {
	float variance = 0.0;
	float sum = 0.0;
	for(int i=0;i<vals;i++) {
		sum = sum + pow((valarr[i] - avg),2);
	}
	variance = sum / (float)(vals - 1);
	return variance;
}

void parse_message_header(int sockfd, struct header *hdr) {
	if(recv(sockfd, hdr, sizeof(struct header),MSG_WAITALL) < sizeof(struct header)) {
		printf("parse_message_header: Recv failed\n");
		exit(EXIT_FAILURE);
	};
}
void handle_open_message(int sockfd, struct header *hdr, struct open_message *rx_open_msg) {
	
	uint16_t rx_bytes = sizeof(struct open_message);
	char buf[4096];
	//Read the fixed length open message from the socket first, then the variable length capabilities
	if(read(sockfd, rx_open_msg, rx_bytes) < rx_bytes) {
		printf("handle_open_message: Open Struct Recv failed\n");
		exit(EXIT_FAILURE);
	};
	rx_bytes = ntohs(hdr->len) - sizeof(struct open_message) - sizeof(struct header);
        if(read(sockfd, buf, rx_bytes) < rx_bytes) {
                printf("handle_open_message: Recv failed\n");
                exit(EXIT_FAILURE);
        };
}
static inline uint16_t get_byte_length(uint16_t prefix_len) {
	if(prefix_len > 24) {
		return 4;
	} else if (prefix_len > 16) {
		return 3;
	} else if (prefix_len > 8) {
		return 2;
	} else if (prefix_len > 0) {
		return 1;
	}
	return 0;
}
void print_ip(char *buf, uint16_t idx, uint16_t prefix_bytes) {

	for(int i = 1; i <= prefix_bytes; i++) {
		printf("%d.",(uint8_t)buf[idx+i]);
	}
	for(int i = 1; i <= (4 - prefix_bytes); i++) {
		if(i == (4 - prefix_bytes)) {
			printf("%d",0);
		} else {
			printf("%d.",0);
		}
	}
	printf("/%d\n",buf[idx]);

}
uint16_t handle_update_message(int sockfd, struct header *hdr, char *recv_buf) {
	uint16_t update_len = (ntohs(hdr->len)) - sizeof(struct header);
	if(recv(sockfd, recv_buf, update_len,MSG_WAITALL) != update_len) {
		printf("handle_update_message: Recv failed\n");
		exit(EXIT_FAILURE);
	}
	uint16_t pa_len = (uint16_t)(ntohs((recv_buf[3] << 8) | recv_buf[2]));
	uint16_t byte_count = update_len;
	uint16_t current_bytes = sizeof(struct update_message) + pa_len;
	uint16_t prefix_bytes = 0;
	uint16_t prefix_count = 0;
	if(pa_len > 0) {
		while((current_bytes) != (byte_count)) {
			prefix_bytes = get_byte_length(recv_buf[current_bytes]);
			current_bytes = prefix_bytes + current_bytes + 1;
			prefix_count++;
		}
	}
	return prefix_count;
}
int main(int argc, char *argv[]) {
	uint16_t cycles = 1;
	int mss = 1460;
	uint32_t local_asn;
	char* destination;
	int option = 0;
	struct timespec start, current, last_keepalive;
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
	memset(recv_buf, 0, sizeof(recv_buf));
	memset(send_buf, 0, sizeof(send_buf));
	int sockfd;

	struct sockaddr_in peer;
	bzero((char *) &peer, sizeof(peer));

	
	bzero(send_buf,sizeof(send_buf));
	peer.sin_family = AF_INET;
	peer.sin_addr.s_addr = inet_addr(destination);
	peer.sin_port = htons(DEFAULT_PORT);        
	
	uint16_t hold_time = 180;
	uint16_t keepalive_interval = (hold_time /3 );
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
	struct capabilities *caps = (struct capabilities *) (datagram + sizeof(struct header) + sizeof(struct open_message));

	hdr->type = 1;
        for(int i = 0; i < 16; i++) {
                hdr->marker[i] = 255;
                keepalive_msg->marker[i] = 255;
                eor_msg->marker[i] = 255;
        };

        open_msg->ver = 4;
        open_msg->my_as = htons(local_asn);
        open_msg->hold_time = htons(hold_time);
        open_msg->bgp_id = 33686018;
        open_msg->optional_params_len = 22;

        caps->option_param_type = 2;
        caps->option_param_len = 20;

        caps->rr_cap_code = 2;
        caps->rr_cap_len = 0;

        caps->err_cap_code = 70;
        caps->err_cap_len = 0;

        caps->as4_as_code = 65;
        caps->as4_as_len = 4;
        caps->as4_as_local_as_1 = 0;
        caps->as4_as_local_as_2 = htons(local_asn);

        caps->mp_cap_code = 1;
        caps->mp_cap_param_len = 4;
        caps->mp_cap_mp_extensions_afi = htons(1);
        caps->mp_cap_mp_extensions_reserved = 0;
        caps->mp_cap_mp_extensions_safi = 1;

        caps->gr_cap_code = 64;
        caps->gr_cap_len = 2;
        caps->gr_cap_timers = htons(16684);
	float diff_time;
	uint64_t update_msg_count;
	uint64_t update_byte_count;
	float updates_per_sec;
	uint32_t prefix_count;	
	uint16_t open_msg_len = sizeof(struct open_message) + sizeof(struct header) + sizeof(struct capabilities);
	hdr->len = htons(open_msg_len);
	
	float total_updates_per_sec;
	float total_diff_time;
	float total_prefix_count;
	float total_update_msg_count;

	float diff_times[cycles];
	float updates_per_sec_arr[cycles];
	printf("Attempting to connect to %s with a local ASN of %d and a MSS of %d for %d cycles\n",destination,local_asn,mss,cycles); 
	uint8_t session;
	for(int current_cycle=0; current_cycle<cycles; current_cycle++) {
		session = 1;
		
		sockfd = socket(AF_INET, SOCK_STREAM, 0);
		if(setsockopt(sockfd, IPPROTO_TCP, TCP_MAXSEG, &mss, sizeof(mss)) < 0) {
			printf("Failed to set socket options: %s\n", strerror(errno));
			exit(EXIT_FAILURE);
		};
		if(connect(sockfd, (struct sockaddr *)&peer, sizeof(peer)) < 0) {	
			printf("Failed to connect: %s\n", strerror(errno));
			exit(EXIT_FAILURE);
		};
		if(send(sockfd, datagram, open_msg_len, 0) < open_msg_len) {
			printf("Failed to send: %s\n", strerror(errno));
			exit(EXIT_FAILURE);
		};
		update_msg_count = 0;
		update_byte_count = 0;	
		struct open_message *rx_open_msg = (struct open_message *)malloc(sizeof(struct open_message));

		struct header *rx_hdr = (struct header *)malloc(sizeof(struct header));
		updates_per_sec = 0.0;
		prefix_count = 0;
		while(session == 1) {
			parse_message_header(sockfd,rx_hdr);
			switch(rx_hdr->type) {
				case 1:
					handle_open_message(sockfd,rx_hdr,rx_open_msg);
					if(rx_open_msg->hold_time < hold_time) {
						hold_time = rx_open_msg->hold_time;
						keepalive_interval = hold_time / 3;
					}
					if(send(sockfd, keepalive_msg, sizeof(struct keepalive_message), 0) < sizeof(struct keepalive_message)) {
						printf("send keepalive failed\n");
						exit(EXIT_FAILURE);
					};
					if(ntohs(rx_open_msg->my_as) == local_asn) {
						printf("Cycle: %d IBGP Established with peer %s in ASN %d with hold_time of %d...", (current_cycle+1),destination, ntohs(rx_open_msg->my_as), ntohs(rx_open_msg->hold_time));
					}  else {
						printf("Cycle: %d EBGP Established with peer %s in ASN %d with hold_time of %d...", (current_cycle+1),destination, ntohs(rx_open_msg->my_as), ntohs(rx_open_msg->hold_time));
					};
					clock_gettime(CLOCK_MONOTONIC_RAW,&last_keepalive);
					if(send(sockfd, eor_msg, sizeof(struct eor_message), 0) < sizeof(struct eor_message)) {
						printf("send EOR message failed\n");
					};
					break;
				case 2:
					if(update_msg_count == 0) {
						clock_gettime(CLOCK_MONOTONIC_RAW,&start);
					};	
					prefix_count = (handle_update_message(sockfd,rx_hdr,recv_buf) + prefix_count);
					update_msg_count++;
					update_byte_count = update_byte_count + ntohs(rx_hdr->len) - sizeof(struct header);
					if(ntohs(rx_hdr->len) == 23) {
						//puts("###EOR Received!!!###");
						//printf("%ld total updates received\n",update_msg_count);
						//printf("%ld total update bytes received\n",update_byte_count);
						printf("Complete!\n");
						clock_gettime(CLOCK_MONOTONIC_RAW,&current);
						diff_time = ((current.tv_sec - start.tv_sec) * 1000000 + (current.tv_nsec - start.tv_nsec) / 1000)/1000000.0;
						//printf("Total time: %f\n", diff_time);
						diff_times[current_cycle] = diff_time;
						updates_per_sec = (float)update_msg_count/diff_time;
						updates_per_sec_arr[current_cycle] = updates_per_sec;
						//printf("Total prefixes received: %d\n",prefix_count);
						total_prefix_count = total_prefix_count + prefix_count;
						total_update_msg_count = total_update_msg_count + update_msg_count;
						total_diff_time = diff_time + total_diff_time;
						total_updates_per_sec = total_updates_per_sec + updates_per_sec;
						close(sockfd);
						session = 0;
					};
					break;
				case 3: 
					printf("Notification message received\n");
					close(sockfd);
					exit(EXIT_FAILURE);	
				case 4:
					break;
				default:
					printf("Illegal message type %d received\n",rx_hdr->type);
					printf("%ld total updates received\n",update_msg_count);
					printf("%ld total update bytes received\n",update_byte_count);
					clock_gettime(CLOCK_MONOTONIC_RAW,&current);
					int diff_time = (current.tv_sec - start.tv_sec) * 1000000 + (current.tv_nsec - start.tv_nsec) / 1000;
					printf("Total time: %d\n", diff_time);
					exit(EXIT_FAILURE);
			}
			clock_gettime(CLOCK_MONOTONIC_RAW,&current);

			if((current.tv_sec - last_keepalive.tv_sec) > keepalive_interval) {
				if(send(sockfd, keepalive_msg, sizeof(struct keepalive_message), 0) < sizeof(struct keepalive_message)) {
					printf("send keepalive failed\n");
					exit(EXIT_FAILURE);
				};
				clock_gettime(CLOCK_MONOTONIC_RAW,&last_keepalive);
				puts("Sending keepalive");
			}
		}
	}
	printf("Completed running %d cycles\n",cycles);
	printf("Average prefix count received per cycle %.2f\n",total_prefix_count/(float)cycles);
	float time_variance = get_variance(diff_times,(total_diff_time/(float)cycles),(sizeof(diff_times)/sizeof(diff_times[0])));
	float update_per_sec_variance = get_variance(updates_per_sec_arr,(total_updates_per_sec/(float)cycles),(sizeof(updates_per_sec_arr)/sizeof(updates_per_sec_arr[0])));
	
	printf("Average time (seconds) per cycle: %f Variance: %f Standard Deviation: %f\n",total_diff_time/(float)cycles,time_variance,sqrt(time_variance));
	
	printf("Average RIB-out per cycle %f Variance: %f Standard Deviation: %f\n",(total_updates_per_sec/(float)cycles),update_per_sec_variance,sqrt(update_per_sec_variance));
	close(sockfd);
	exit(EXIT_SUCCESS);
}
