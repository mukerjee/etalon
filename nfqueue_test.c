#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <linux/types.h>
#include <string.h>
#include <vector>

/* for interrupt handler*/
#include <signal.h>

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

#include <sstream>
#include <iostream>
#include <map>
#include <utility>
#include <string>

int go = 0;
unsigned int NUM_HOSTS  = 0;		
unsigned int NUM_THREADS = 0;
#define MAX_HOSTS 64

pthread_t threads[MAX_HOSTS];
pthread_t sched_thread;
std::map<int, std::pair<std::string, std::string> > host_pair;
std::vector<std::string> host_list;
char PACKET_BW[10] = "1gbit";
char CIRCUIT_BW[10] = "100mbit";
char OTHER_BW[10] = "1gbit";

//Traffic matrix
std::map< std::string, std::map<std::string, unsigned int> > traffic_matrix;

FILE *fp;

void printTM() {
    system("clear");
    for (unsigned int i=0; i<host_list.size(); i++) {
        for (unsigned int j=0; j<host_list.size(); j++) {
            printf("%6u ",traffic_matrix[host_list[i]][host_list[j]]);
        }
        printf("\n");
    }
}

void setPath (std::string src, std::string dst, int cls) {
	char cmd[512];
	sprintf(cmd, "sudo iptables -t mangle -A POSTROUTING -o eth0 -s %s -d %s -j CLASSIFY --set-class 1:%d",
			src.c_str(), dst.c_str(), cls+1);
	system(cmd);
}

void initPath() {
	for (unsigned int i=0; i<NUM_HOSTS; i++) {
		for (unsigned int j=0; j<NUM_HOSTS; j++) {
			if (i == j) {
				continue;
			}
			// using packet path by default
			setPath (host_list[i], host_list[j], j);
		}
	}
}	

void initTM() {
	for (unsigned int i=0; i<NUM_HOSTS; i++) {
		for (unsigned int j=0; j<NUM_HOSTS; j++) {
			traffic_matrix[host_list[i]][host_list[j]] = 0;
		}
	}
}

void clearTC() {
	system ("sudo tc qdisc del dev eth0 root");
}

void initTC() {
	clearTC();

	char cmd[512];
	system("tc qdisc add dev eth0 root handle 1: htb default 201");
	sprintf(cmd, "tc class add dev eth0 parent 1: classid 1:201 htb rate %s ceil %s", OTHER_BW, OTHER_BW);
	system(cmd);

	for (unsigned int i=1; i<=NUM_HOSTS; i++) {
		sprintf(cmd, "tc class add dev eth0 parent 1: classid 1:%d htb rate %s ceil %s",
				i, PACKET_BW, PACKET_BW);
		system (cmd);
	}

	for (unsigned int i=101; i<=100+NUM_HOSTS; i++) {
		sprintf(cmd, "tc class add dev eth0 parent 1: classid 1:%d htb rate %s ceil %s",
				i, CIRCUIT_BW, CIRCUIT_BW);
		system (cmd);
	}
	printf("============TC initialized===========\n");
	system("tc qdisc show");
	system("tc class show dev eth0");
}

void clearIPT() {
	int queue_num = 1;
	for (unsigned int i=0; i<NUM_HOSTS; i++) {
		for (unsigned int j=0; j<NUM_HOSTS; j++) {
			if (i == j) {
				continue;
			}
			char cmd[512];
			sprintf(cmd, "sudo iptables -D FORWARD -s %s -d %s -j NFQUEUE --queue-num %d", host_list[i].c_str(), host_list[j].c_str(), queue_num);
			system(cmd);
			host_pair[queue_num++] = std::make_pair(host_list[i], host_list[j]);

		}
	}		
}

void initIPT () {
	clearIPT();
	int queue_num = 1;
	for (unsigned int i=0; i<NUM_HOSTS; i++) {
		for (unsigned int j=0; j<NUM_HOSTS; j++) {
			if (i == j) {
				continue;
			}
			char cmd[512];
			sprintf(cmd, "sudo iptables -I FORWARD -s %s -d %s -j NFQUEUE --queue-num %d", host_list[i].c_str(), host_list[j].c_str(), queue_num);
			system(cmd);

			host_pair[queue_num++] = std::make_pair(host_list[i], host_list[j]);

		}
	}		
	printf("=============== NFQUEUE initialized ================\n");
	system("sudo iptables -nvL");
}


void getNumQueuedPkt (){//u_int16_t queue_id) {
	char *token;
	char buffer[102400]; 
	size_t bytes_read;
	char id[3];
	char queue_len[10];
	rewind(fp);
	bytes_read = fread (buffer, 1, sizeof (buffer), fp);
	buffer[bytes_read] = '\0';
	char* new_str = strdup(buffer);
	while ((token = strsep(&new_str, "\n")) != NULL) {
		sscanf(token,"%s %*s %s", id, queue_len);
        int num_queue = atoi(id);
        traffic_matrix[host_pair[num_queue].first][host_pair[num_queue].second] = atoi(queue_len);

		//if (atoi(id) == queue_id) 
		//	return atoi(queue_len);
	}
	//return -1;
}

u_int32_t analyzePacket(struct nfq_data *tb) {

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
	//printf("SIZE: %d\n",nfq_get_payload(tb, &data));
//	if (ret >= 0) {
//
//		printf("Packet Received: %d \n", ret);
//
//		/* extracting the ipheader from packet */
//		struct sockaddr_in source, dest;
//
//		struct iphdr *iph = ((struct iphdr *) data);
//
//		memset(&source, 0, sizeof(source));
//		source.sin_addr.s_addr = iph->saddr;
//
//		memset(&dest, 0, sizeof(dest));
//		dest.sin_addr.s_addr = iph->daddr;
//
//		printf("|-Source IP: %s\n", inet_ntoa(source.sin_addr));
//		printf("|-Destination IP: %s\n", inet_ntoa(dest.sin_addr));
//
//	}
	//return the queue id
	return id;
}

int packetHandler(struct nfq_q_handle *qh, struct nfgenmsg *nfmsg, struct nfq_data *nfa,
		void *data) {

	u_int16_t queue_num = ntohs(nfmsg->res_id);
	//printf("entering callback\n");
	//when to drop
	//usleep(10000);

	//analyze the packet and return the packet id in the queue
	u_int32_t id = analyzePacket(nfa);
	while (!go){
		usleep(1);
		//printf("no send\n");;
	}	
	//printf("Src: %s\tDest: %s %d\n",host_pair[queue_num].first.c_str(), host_pair[queue_num].second.c_str(),id );

	//this is the point where we decide the destiny of the packet
	//printf("send\n");
	return nfq_set_verdict(qh, id, NF_ACCEPT, 0, NULL);
}

void *SchedThread(void *threadid) {
	while (1) {
        usleep(3000);
        getNumQueuedPkt ();
	//do schedule here
        printTM();
	if (go == 0)
		go = 1;
	else
		go = 0;
	//printf("%d\n",go);
    }
	return NULL;	
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

	printf("open handle to the netfilter_queue - > Thread: %ld \n", tid); h = nfq_open();
	if (!h) {
		fprintf(stderr, "cannot open nfq_open()\n");
		return NULL;
	}
	//increase the recv buffer size of nfqueue
	nfnl_rcvbufsiz(nfq_nfnlh(h), sizeof(buf)*1024);

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
	if (nfq_set_mode(qh, NFQNL_COPY_PACKET, 0xfffff) < 0) {
		fprintf(stderr, "can't set packet_copy mode\n");
		return NULL;
	}

	//getting the file descriptor
	fd = nfq_fd(h);

	while ((rv = recv(fd, buf, sizeof(buf), 0))) {
		if (rv < 0)
			continue;
		//printf("pkt received in Thread: %ld %d\n", tid, rv);
        	//traffic_matrix[host_pair[tid].first][host_pair[tid].second] += (rv-88);
		nfq_handle_packet(h, buf, rv);
	}

	printf("unbinding from queue Thread: %ld  \n", tid);
	nfq_destroy_queue(qh);

	printf("closing library handle\n");
	nfq_close(h);

	return NULL;

}

void init() {

	int read;
	char *line = NULL;
	size_t len = 0; 
	FILE *f_host = fopen("./hosts.txt","r");
	while ((read=getline(&line, &len, f_host))!=-1) {
		char host[20];
		sscanf(line,"%s", host);
		host_list.push_back(std::string(host));
	} 
	fclose(f_host);
	NUM_HOSTS = host_list.size();

	initTC();
	initIPT();
	initPath();
	
}

void 
intHandler(int signum) {
	//destroy all threads
	//pthread_exit(NULL);
	clearIPT();
	clearTC();
	fclose(fp);
	exit(0);
}


int main(int argc, char *argv[]) {

	signal(SIGINT, intHandler);
	//signal(SIGTERM, intHandler);
	//set process priority
	setpriority(PRIO_PROCESS, 0, -20);
	init();

	int rc;
	long balancerSocket;
	NUM_THREADS = NUM_HOSTS*(NUM_HOSTS-1);

	for (balancerSocket = 1; balancerSocket <= NUM_THREADS; balancerSocket++) {
		printf("In main: creating thread %ld\n", balancerSocket);

		//send the balancer socket for the queue
		rc = pthread_create(&threads[balancerSocket], NULL, QueueThread,
				(void *) balancerSocket);

		if (rc) {
			printf("ERROR; return code from pthread_create() is %d\n", rc);
			exit(-1);
		}
	}
	sleep(2);
	printf("In main: creating scheduler thread\n");

	//send the balancer socket for the queue
	rc = pthread_create(&sched_thread, NULL, SchedThread,
			(void *) 0);

	if (rc) {
		printf("ERROR; return code from pthread_create() is %d\n", rc);
		exit(-1);
	}

	fp = fopen("/proc/net/netfilter/nfnetlink_queue","r");
	if (fp == NULL) {
		perror("Failed to open /proc/net/netfilter/nfnetlink_queue");
		exit(1);
	}

	while (1) {
		sleep(10);
	}

}
