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

u_int32_t analyzePacket(struct nfq_data *tb, int *blockFlag) {

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

	printf("open handle to the netfilter_queue - > Thread: %ld \n", tid);
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
	printf("binding this socket to queue '%ld'\n", tid);
	qh = nfq_create_queue(h, tid, &packetHandler, NULL);
	if (!qh) {
		fprintf(stderr, "error during nfq_create_queue()\n");
		return NULL;
	}

	//set queue length before start dropping packages
	ql = nfq_set_queue_maxlen(qh, 100000);
	if (ql == -1)
		perror("nfq_set_queue_maxlen");

	//set the queue for copy mode
	if (nfq_set_mode(qh, NFQNL_COPY_PACKET, sizeof(struct iphdr)) < 0) {
		fprintf(stderr, "can't set packet_copy mode\n");
		return NULL;
	}

	//getting the file descriptor
	fd = nfq_fd(h);

	while ((rv = recv(fd, buf, sizeof(buf), 0)) && rv >= 0) {
		printf("pkt received in Thread: %ld\n", tid);
		nfq_handle_packet(h, buf, rv);
	}

	printf("unbinding from queue Thread: %ld  \n", tid);
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
