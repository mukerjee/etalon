#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <linux/types.h>
#include <string.h>
#include <vector>
#include <queue>

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
#include <algorithm>

struct _pkt_queue {
    int _id;
    std::queue<std::pair<char*, int> > _queue;
};

int stop = 0;
unsigned int max_demand = 0;
unsigned int NUM_HOSTS  = 0;		
unsigned int NUM_THREADS = 0;
#define MAX_HOSTS 64

pthread_t threads[MAX_HOSTS*MAX_HOSTS];
pthread_t xmit_thread[MAX_HOSTS*MAX_HOSTS];
pthread_t sched_thread;
struct nfq_handle *h[MAX_HOSTS*MAX_HOSTS];

std::map<int, std::pair<int, int> > host_pair;
int host_to_queueid[MAX_HOSTS][MAX_HOSTS];
std::vector<std::string> host_list;
const char PACKET_BW[10] = "10mbit";
const char CIRCUIT_BW[10] = "100mbit";
const char OTHER_BW[10] = "1gbit";

int packet_cls[MAX_HOSTS];
int circuit_cls[MAX_HOSTS][MAX_HOSTS];

//Traffic matrix
uint64_t traffic_matrix[MAX_HOSTS][MAX_HOSTS];
uint64_t traffic_matrix_pkt[MAX_HOSTS][MAX_HOSTS];
std::map< int, std::queue<std::pair<char*, int> > > pkt_queue;

FILE *fp;

void printTM() {
    unsigned int max = 0;
    system("clear");
    for (unsigned int i=0; i<host_list.size(); i++) {
        for (unsigned int j=0; j<host_list.size(); j++) {
            if (max_demand < traffic_matrix_pkt[i][j])
                max_demand  = traffic_matrix_pkt[i][j];
            printf("%6lu ",traffic_matrix_pkt[i][j]);
        }
        printf("\n");
    }
    printf("MAX: %u\n", max_demand);
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
    int qnum = 1;
    for (unsigned int i=0; i<NUM_HOSTS; i++) {
        for (unsigned int j=0; j<NUM_HOSTS; j++) {
            traffic_matrix[i][j] = 0;
            traffic_matrix_pkt[i][j] = 0;
            std::queue< std::pair<char*, int> >  empty2;
            std::swap(pkt_queue[qnum++], empty2);
        }
    }

}

void clearTC() {
    system ("sudo tc qdisc del dev eth0 root");
}

void initTC() {
    clearTC();

    char cmd[512];
    system("tc qdisc add dev eth0 root handle 1: htb default 65");
    sprintf(cmd, "tc class add dev eth0 parent 1: classid 1:65 htb rate %s ceil %s", OTHER_BW, OTHER_BW);
    system(cmd);


    for (unsigned int i=0; i<NUM_HOSTS; i++) {
        sprintf(cmd, "tc class add dev eth0 parent 1: classid 1:%d htb rate %s ceil %s",
                i, PACKET_BW, PACKET_BW);
        packet_cls[i] = i;
        system (cmd);
    }

    int cls = 100;
    for (unsigned int i=0; i<NUM_HOSTS; i++) {
        for (unsigned int j=0; i<NUM_HOSTS; i++) {
            if (i==j)
                continue;
            sprintf(cmd, "tc class add dev eth0 parent 1: classid 1:%d htb rate %s ceil %s",
                    cls, CIRCUIT_BW, CIRCUIT_BW);
            system (cmd);
            circuit_cls[i][j] = cls++;
        }
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
            sprintf(cmd, "sudo iptables -D FORWARD -s %s -d %s -j NFQUEUE --queue-num %d", host_list[i].c_str(), host_list[j].c_str(), queue_num++);
            system(cmd);
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
            host_to_queueid[i][j] = queue_num;
            system(cmd);

            host_pair[queue_num++] = std::make_pair(i, j);

        }
    }		
    printf("=============== NFQUEUE initialized ================\n");
    system("sudo iptables -nvL");
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

    return id;
}

int packetHandler(struct nfq_q_handle *qh, struct nfgenmsg *nfmsg, struct nfq_data *nfa,
        void *data) {

    u_int16_t queue_num = ntohs(nfmsg->res_id);

    u_int32_t id = analyzePacket(nfa);
    return nfq_set_verdict(qh, id, NF_ACCEPT, 0, NULL);
}

void *xmitThread(void *_queue) {
    struct _pkt_queue *pkt_queue = (struct _pkt_queue*)_queue;
    int queue_id = pkt_queue->_id;
    while(!pkt_queue->_queue.empty()) {
        nfq_handle_packet(h[queue_id], pkt_queue->_queue.front().first, pkt_queue->_queue.front().second);
        pkt_queue->_queue.pop();
    }
    return NULL;
}

void *SchedThread(void *threadid) {
    uint64_t tmp_TM[NUM_HOSTS][NUM_HOSTS];
    uint64_t tmp_TM_pkt[NUM_HOSTS][NUM_HOSTS];
    //std::map< int, std::queue<std::pair<char*, int> > > tmp_pkt_queue;
    struct _pkt_queue tmp_pkt_queue[MAX_HOSTS*MAX_HOSTS];

    while (1) {
        usleep(3000);
        stop = 1;
        //Take a snapshot of TM
        for (int i=0; i<NUM_HOSTS; i++) {
            for (int j=0; j<NUM_HOSTS; j++) {
                if (i==j) continue;
                int queue_id = host_to_queueid[i][j];
                tmp_TM[i][j] = traffic_matrix[i][j];
                tmp_pkt_queue[queue_id]._id = queue_id;
                tmp_pkt_queue[queue_id]._queue = pkt_queue[queue_id];
            }
        }
        initTM();
        //printTM();
        stop = 0;
        //call solstice
        // 
        //set tc
        //transmit
        int rc;
        for (int i=1; i<=NUM_THREADS; i++) {
            if (tmp_pkt_queue[i]._queue.size() == 0) continue;
            while(!tmp_pkt_queue[i]._queue.empty()) {
                nfq_handle_packet(h[tmp_pkt_queue[i]._id], tmp_pkt_queue[i]._queue.front().first, tmp_pkt_queue[i]._queue.front().second);
                tmp_pkt_queue[i]._queue.pop();
            }
            /*rc = pthread_create(&xmit_thread[i], NULL, xmitThread,
              &tmp_pkt_queue[i]);

              if (rc) {
              printf("ERROR; return code from pthread_create() is %d\n", rc);
              exit(-1);

              }*/
        }
    }
    return NULL;	
}

void *QueueThread(void *threadid) {

    //thread id
    long tid;
    tid = (long) threadid;

    struct nfq_q_handle *qh;
    char buf[128000] __attribute__ ((aligned));

    //pointers and descriptors
    int fd;
    int rv;
    int ql;

    printf("open handle to the netfilter_queue - > Thread: %ld \n", tid); 
    h[tid] = nfq_open();
    if (!h[tid]) {
        fprintf(stderr, "cannot open nfq_open()\n");
        return NULL;
    }
    //increase the recv buffer size of nfqueue
    nfnl_rcvbufsiz(nfq_nfnlh(h[tid]), sizeof(buf)*1024);

    //unbinding previous procfs
    if (nfq_unbind_pf(h[tid], AF_INET) < 0) {
        fprintf(stderr, "error during nfq_unbind_pf()\n");
        return NULL;
    }

    //binding the netlink procfs
    if (nfq_bind_pf(h[tid], AF_INET) < 0) {
        fprintf(stderr, "error during nfq_bind_pf()\n");
        return NULL;
    }

    //connet the thread for specific socket
    printf("binding this socket to queue '%ld'\n", tid);
    qh = nfq_create_queue(h[tid], tid, &packetHandler, NULL);
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
    fd = nfq_fd(h[tid]); 
    while ((rv = recv(fd, buf, sizeof(buf), 0))) {
        if (rv < 0)
            continue;
        while (stop == 1) {}
        //printf("pkt received in Thread: %ld %d\n", tid, rv);
        traffic_matrix[host_pair[tid].first][host_pair[tid].second] += (rv-88); //only payload size
        traffic_matrix_pkt[host_pair[tid].first][host_pair[tid].second] ++; //only payload size
        char* pkt = (char*)malloc(rv);
        memcpy(pkt, buf, rv);
        pkt_queue[tid].push(std::make_pair(pkt, rv));
    }

    printf("unbinding from queue Thread: %ld  \n", tid);
    nfq_destroy_queue(qh);

    printf("closing library handle\n");
    nfq_close(h[tid]);

    return NULL;

}

void init() {

    int read;
    char *line = NULL;
    size_t len = 0; 
    FILE *f_host = fopen("~/hosts.txt","r");
    while ((read=getline(&line, &len, f_host))!=-1) {
        char host[20];
        sscanf(line,"%s", host);
        host_list.push_back(std::string(host));
    } 
    fclose(f_host);
    NUM_HOSTS = host_list.size();

    initTM();
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
    sleep(1);
    printf("In main: creating scheduler thread\n");

    rc = pthread_create(&sched_thread, NULL, SchedThread,
            (void *) 0);

    if (rc) {
        printf("ERROR; return code from pthread_create() is %d\n", rc);
        exit(-1);
    }

    while (1) {
        sleep(10);
    }

}
