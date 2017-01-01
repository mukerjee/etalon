// * Notes:
//complex implementation of libnetfilter_queue library,
//using different threads for each queue, depends the iptables rules
//for example:
//#: iptables -A INPUT  -j NFQUEUE --queue-balance 0:3
//#: iptables -A OUTPUT  -j NFQUEUE --queue-balance 4:8
//the main function create array of threads, each threads
//listen to different queue, when packet arrived to the queue
//a callback function call and start to analyze the packet,
//if the payload contain specific word or stream it's drop the packet
//* Proxytype.blogspot.com *

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <linux/types.h>
#include <string.h>

/* for ethernet header */
#include<net/ethernet.h>

/* for UDP header */
#include<linux/udp.h>

/* for TCP header */
#include<linux/tcp.h>

/* for IP header */
#include<linux/ip.h>

/*  -20 (maximum priority) */
#include <sys/time.h>
#include <sys/resource.h>

/* for NF_ACCEPT */
#include <linux/netfilter.h>

/* for Threads */
#include <pthread.h>

/* for Queue */
#include <libnetfilter_queue/libnetfilter_queue.h>

#define NUM_THREADS     2 //15

pthread_t threads[NUM_THREADS];

void printTCP(unsigned char *buffer) {

	unsigned short iphdrlen;

	struct iphdr *iph = (struct iphdr *) (buffer + sizeof(struct ethhdr));
	iphdrlen = iph->ihl * 4;

	struct tcphdr *tcph = (struct tcphdr *) (buffer + iphdrlen
			+ sizeof(struct ethhdr));

	int header_size = sizeof(struct ethhdr) + iphdrlen + tcph->doff * 4;

	printf("| Packet Type: TCP \n");
	printf("|-Source Port      : %u\n", ntohs(tcph->source));
	printf("|-Destination Port : %u\n", ntohs(tcph->dest));
	printf("|-Sequence Number    : %u\n", ntohl(tcph->seq));
	printf("|-Acknowledge Number : %u\n", ntohl(tcph->ack_seq));
	printf("|-Header Length      : %d DWORDS or %d BYTES\n",
			(unsigned int) tcph->doff, (unsigned int) tcph->doff * 4);
	printf("|-CWR Flag : %d\n", (unsigned int) tcph->cwr);
	printf("|-ECN Flag : %d\n", (unsigned int) tcph->ece);
	printf("|-Urgent Flag          : %d\n", (unsigned int) tcph->urg);
	printf("|-Acknowledgement Flag : %d\n", (unsigned int) tcph->ack);
	printf("|-Push Flag            : %d\n", (unsigned int) tcph->psh);
	printf("|-Reset Flag           : %d\n", (unsigned int) tcph->rst);
	printf("|-Synchronise Flag     : %d\n", (unsigned int) tcph->syn);
	printf("|-Finish Flag          : %d\n", (unsigned int) tcph->fin);
	printf("|-Window         : %d\n", ntohs(tcph->window));
	printf("|-Checksum       : %d\n", ntohs(tcph->check));
	printf("|-Urgent Pointer : %d\n", tcph->urg_ptr);
}

void printUDP(unsigned char *buffer) {
	unsigned short iphdrlen;

	struct iphdr *iph = (struct iphdr *) (buffer + sizeof(struct ethhdr));
	iphdrlen = iph->ihl * 4;

	struct udphdr *udph = (struct udphdr*) (buffer + iphdrlen
			+ sizeof(struct ethhdr));

	int header_size = sizeof(struct ethhdr) + iphdrlen + sizeof udph;

	printf("| Packet Type: UDP \n");
	printf("|-Source Port      : %u\n", ntohs(udph->source));
	printf("|-Destination Port : %u\n", ntohs(udph->dest));
	printf("|-UDP Length : %u\n", ntohs(udph->len));
	printf("|-UDP Checksum : %u\n", ntohs(udph->check));

}

char * getText(unsigned char * data, char Size) {

	char * text = malloc(Size);
	int i = 0;

	for (i = 0; i < Size; i++) {
		if (data[i] >= 32 && data[i] <= 128)
			text[i] = (unsigned char) data[i];
		else
			text[i] = '.';
	}
	return text;

}

u_int32_t analyzePacket(struct nfq_data *tb, int *blockFlag) {

	printf("analyzePacket\n");
	//packet id in the queue
	int id = 0;

	//the queue header
	struct nfqnl_msg_packet_hdr *ph;

	//the packet
	unsigned char *data;

	//packet size
	int ret;

	//extracting the queue header
	ph = nfq_get_msg_packet_hdr(tb);

	//getting the id of the packet in the queue
	if (ph)
		id = ntohl(ph->packet_id);

	//get the length and the payload of the packet
	ret = nfq_get_payload(tb, &data);
	if (ret >= 0) {

		printf("Packet Received: %d \n", ret);

		/* extracting the ipheader from packet */
		struct sockaddr_in source, dest;

		struct iphdr *iph = ((struct iphdr *) data);

		memset(&source, 0, sizeof(source));
		source.sin_addr.s_addr = iph->saddr;

		memset(&dest, 0, sizeof(dest));
		dest.sin_addr.s_addr = iph->daddr;

		printf("|-Source IP: %s\n", inet_ntoa(source.sin_addr));
		printf("|-Destination IP: %s\n", inet_ntoa(dest.sin_addr));
		printf("|-Checking for Protocol: \n");

		if (iph->protocol == 6) {
			printTCP(data);
		} else if (iph->protocol == 17) {
			printUDP(data);
		}

	}
	//return the queue id
	return id;

}

int packetHandler(struct nfq_q_handle *qh, struct nfgenmsg *nfmsg, struct nfq_data *nfa,
		void *data) {

	printf("entering callback \n");

	//when to drop
	int blockFlag = 0;

	//analyze the packet and return the packet id in the queue
	u_int32_t id = analyzePacket(nfa, &blockFlag);

	//this is the point where we decide the destiny of the packet
	if (blockFlag == 0)
		return nfq_set_verdict(qh, id, NF_ACCEPT, 0, NULL);
	else
		return nfq_set_verdict(qh, id, NF_DROP, 0, NULL);



}

void *QueueThread(void *threadid) {

	//thread id
	long tid;
	tid = (long) threadid;


	struct nfq_handle *h;
	struct nfq_q_handle *qh;
	char buf[128000] __attribute__ ((aligned));

	//pointers and descriptors
	int fd;
	int rv;
	int ql;

	printf("open handle to the netfilter_queue - > Thread: %d \n", tid);
	h = nfq_open();
	if (!h) {
		fprintf(stderr, "cannot open nfq_open()\n");
		return NULL;
	}

	//unbinding previous procfs
	if (nfq_unbind_pf(h, AF_INET) < 0) {
		fprintf(stderr, "error during nfq_unbind_pf()\n");
		return NULL;
	}

	//binding the netlink procfs
	if (nfq_bind_pf(h, AF_INET) < 0) {
		fprintf(stderr, "error during nfq_bind_pf()\n");
		return NULL;
	}

	//connet the thread for specific socket
	printf("binding this socket to queue '%d'\n", tid);
	qh = nfq_create_queue(h, tid, &packetHandler, NULL);
	if (!qh) {
		fprintf(stderr, "error during nfq_create_queue()\n");
		return NULL;
	}

	//set queue length before start dropping packages
	ql = nfq_set_queue_maxlen(qh, 100000);

	//set the queue for copy mode
	if (nfq_set_mode(qh, NFQNL_COPY_PACKET, 0xffff) < 0) {
		fprintf(stderr, "can't set packet_copy mode\n");
		return NULL;
	}

	//getting the file descriptor
	fd = nfq_fd(h);

	while ((rv = recv(fd, buf, sizeof(buf), 0)) && rv >= 0) {
		printf("pkt received in Thread: %d \n", tid);
		nfq_handle_packet(h, buf, rv);
	}

	printf("unbinding from queue Thread: %d  \n", tid);
	nfq_destroy_queue(qh);

	printf("closing library handle\n");
	nfq_close(h);

	return NULL;

}

int main(int argc, char *argv[]) {

	//set process priority
	setpriority(PRIO_PROCESS, 0, -20);

	int rc;
	long balancerSocket;
	for (balancerSocket = 0; balancerSocket < NUM_THREADS; balancerSocket++) {
		printf("In main: creating thread %ld\n", balancerSocket);

		//send the balancer socket for the queue
		rc = pthread_create(&threads[balancerSocket], NULL, QueueThread,
				(void *) balancerSocket);

		if (rc) {
			printf("ERROR; return code from pthread_create() is %d\n", rc);
			exit(-1);
		}
	}

	while (1) {
		sleep(10);
	}

	//destroy all threads
	pthread_exit(NULL);
}
