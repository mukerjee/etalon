#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
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

#include "sols.h"

#include "ctpl.h"

#ifdef __APPLE__
#define FMT_U64 "%llu"
#else
#define FMT_U64 "%lu"
#endif
struct _pkt_queue {
    int _id;
    int round;
    double circuit_bytes;
    std::queue<std::pair<char*, int> > _queue;
};

int packet_flag = 0;

int stop = 0;
unsigned int max_demand = 0;
unsigned int NUM_HOSTS  = 0;		
unsigned int NUM_THREADS = 0;
#define MAX_HOSTS 64
#define MAX_ROUND 64

pthread_t threads[MAX_HOSTS*MAX_HOSTS];
pthread_t xmit_thread[MAX_HOSTS*MAX_HOSTS];
pthread_t sched_thread;

/*mutexes*/
pthread_mutex_t queue_mutex[MAX_HOSTS*MAX_HOSTS];
pthread_mutex_t pkt_queue_mutex[MAX_HOSTS*MAX_HOSTS][MAX_ROUND];
pthread_mutex_t ipt_mutex;
pthread_mutex_t ids_mutex[MAX_HOSTS*MAX_HOSTS];

struct nfq_handle *h[MAX_HOSTS*MAX_HOSTS];

std::map<int, std::pair<int, int> > host_pair;
int host_to_queueid[MAX_HOSTS][MAX_HOSTS];
std::vector<std::string> host_list;
const char PACKET_BW[10] = "10mbit";
const char CIRCUIT_BW[10] = "100mbit";
const char OTHER_BW[10] = "1gbit";
double circuit_bw = 125; //1.25 Bytes/usec == 100 mbps
double packet_bw = 12.5; //0.125 Bytes/usec == 10 mbps

double circuit_rate[MAX_HOSTS][MAX_HOSTS];

int packet_cls[MAX_HOSTS];
int circuit_cls[MAX_HOSTS][MAX_HOSTS];
int current_cls[MAX_HOSTS][MAX_HOSTS];
int current_path[MAX_HOSTS][MAX_HOSTS];
int ipt_rules[MAX_HOSTS][MAX_HOSTS]; 

std::vector<uint32_t> ids [MAX_HOSTS*MAX_HOSTS];

//Traffic matrix
uint64_t traffic_matrix[MAX_HOSTS][MAX_HOSTS];
uint64_t traffic_matrix_pkt[MAX_HOSTS][MAX_HOSTS];
std::map< int, std::queue<std::pair<char*, int> > > pkt_queue;

//solstice
sols_t s;

FILE *fp;

void init_sols();

static void
mset(sols_mat_t * m, uint64_t *d) {
    int i, j;
    uint64_t v;

    for (i = 0; i < m->nhost; i++) {
        for (j = 0; j < m->nhost; j++) {
            v = d[i*m->nhost + j];
            if (v == 0) continue;
            sols_mat_set(m, i, j, v);
        }
    }
}

void printTM(uint64_t *tmp_TM) {
    unsigned int max = 0;
    for (unsigned int i=0; i<NUM_HOSTS; i++) {
        for (unsigned int j=0; j<NUM_HOSTS; j++) {
            if (max_demand < tmp_TM[i * NUM_HOSTS + j])
                max_demand  = tmp_TM[i * NUM_HOSTS + j];
            fprintf(fp, "%6lu ",tmp_TM[i * NUM_HOSTS + j]);
        }
        fprintf(fp,"\n");
    }
    fprintf(fp, "MAX: %u\n\n\n", max_demand);
}

void setPath (int src, int dst, int cls) {
    char cmd[512];
    sprintf(cmd, "sudo iptables -R POSTROUTING %d -t mangle -o eth0 -s %s -d %s -j CLASSIFY --set-class 1:%d",
            ipt_rules[src][dst], host_list[src].c_str(), host_list[dst].c_str(), cls);

    pthread_mutex_lock(&ipt_mutex);
    system(cmd);
    pthread_mutex_unlock(&ipt_mutex);
}


void initPath() {
    int rule_num = 1;
    for (unsigned int i=0; i<NUM_HOSTS; i++) {
        for (unsigned int j=0; j<NUM_HOSTS; j++) {
            if (i == j) {
                continue;
            }
            char cmd[512];
            sprintf(cmd, "sudo iptables -t mangle -A POSTROUTING -o eth0 -s %s -d %s -j CLASSIFY --set-class 1:%d",
                    host_list[i].c_str(), host_list[j].c_str(), j+1);
            system(cmd);
            ipt_rules[i][j] = rule_num++; 
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

void setTC(int cls, double bw) {
    char cmd[512];

    sprintf(cmd, "tc class change dev eth0 parent 1: classid 1:%d htb rate %fmbit ceil %fmbit",
            cls, bw, bw);

    system (cmd);
}

void initTC() {
    clearTC();

    char cmd[512];
    system("tc qdisc add dev eth0 root handle 1: htb default 65");
    sprintf(cmd, "tc class add dev eth0 parent 1: classid 1:65 htb rate %s ceil %s", OTHER_BW, OTHER_BW);
    system(cmd);

    for (unsigned int i=0; i<NUM_HOSTS; i++) {
        sprintf(cmd, "tc class add dev eth0 parent 1: classid 1:%d htb rate %s ceil %s",
                i+1, PACKET_BW, PACKET_BW);
        packet_cls[i] = i+1;
        system (cmd);
    }

    int cls = 101;
    for (unsigned int i=0; i<NUM_HOSTS; i++) {
        for (unsigned int j=0; j<NUM_HOSTS; j++) {
            if (i==j)
                continue;
            sprintf(cmd, "tc class add dev eth0 parent 1: classid 1:%d htb rate %s ceil %s",
                    cls, CIRCUIT_BW, CIRCUIT_BW);
            system (cmd);
            circuit_cls[i][j] = cls++;
        }
    }
    /*printf("============TC initialized===========\n");
    system("tc qdisc show");
    system("tc class show dev eth0");*/
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
    system("sudo iptables -t mangle -F");
    system("sudo iptables -t mangle -X");
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
    /*printf("=============== NFQUEUE initialized ================\n");
    system("sudo iptables -nvL");*/
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

    if (qh == NULL || nfmsg == NULL || nfa == NULL)
        printf("nfmsg: %p, qh: %p, nfq_data: %p\n", qh, nfmsg, nfa);
    u_int16_t queue_num = ntohs(nfmsg->res_id);

    u_int32_t id = analyzePacket(nfa);
    //ids[queue_num].push_back(id);
    return nfq_set_verdict(qh, id, NF_ACCEPT, 0, NULL);
}

void xmitThread(int id, struct _pkt_queue* pkt_queue) {
    int queue_id = pkt_queue->_id;
    int src = host_pair[queue_id].first;
    int dest = host_pair[queue_id].second;
    int sent_byte = 0;
    int round = pkt_queue->round;

    pthread_mutex_lock(&pkt_queue_mutex[queue_id][round]);
    if (current_path[src][dest] == 1) {
        double cir_bytes = pkt_queue->circuit_bytes;
        int cnt = 1;
        while(!pkt_queue->_queue.empty()) {
            int payload_byte = pkt_queue->_queue.front().second - 88;
            if (sent_byte + payload_byte > cir_bytes) 
                break;
            nfq_handle_packet(h[queue_id], pkt_queue->_queue.front().first, pkt_queue->_queue.front().second);
            pkt_queue->_queue.pop();
            sent_byte += payload_byte;
            cnt++;
        }
    }
    if (pkt_queue->_queue.empty() == false  && current_path[src][dest] == 1) {
        setPath(host_pair[queue_id].first, host_pair[queue_id].second, packet_cls[host_pair[queue_id].second]);
        current_path[src][dest] = 0;
    }
    while(!pkt_queue->_queue.empty()) {
        nfq_handle_packet(h[queue_id], pkt_queue->_queue.front().first, pkt_queue->_queue.front().second);
        pkt_queue->_queue.pop();
    }
    pthread_mutex_unlock(&pkt_queue_mutex[queue_id][round]);
}

void *SchedThread(void *threadid) {
    uint64_t tmp_TM [NUM_HOSTS*NUM_HOSTS];
    uint64_t tmp_TM_pkt[NUM_HOSTS][NUM_HOSTS];
    struct _pkt_queue tmp_pkt_queue[NUM_HOSTS*NUM_HOSTS][MAX_ROUND];
    int cur_round = 0;
    ctpl::thread_pool p(NUM_HOSTS*NUM_HOSTS);
    while (1) {
        usleep(s.week_len);
        cur_round++;
        cur_round %= MAX_ROUND;
        
        //Take a snapshot of TM
        for (unsigned int i=0; i<NUM_HOSTS; i++) {
            for (unsigned int j=0; j<NUM_HOSTS; j++) {
                if (i==j) continue;
                int queue_id = host_to_queueid[i][j];
                pthread_mutex_lock(&queue_mutex[queue_id]);
                pthread_mutex_lock(&pkt_queue_mutex[queue_id][cur_round]);
                tmp_TM[i * NUM_HOSTS + j] = traffic_matrix[i][j];
                tmp_pkt_queue[queue_id][cur_round]._id = queue_id;
                if (tmp_pkt_queue[queue_id][cur_round]._queue.size() != 0) {
                    printf("[ERROR] %d %d %lu\n",cur_round, queue_id,tmp_pkt_queue[queue_id][cur_round]._queue.size());
                    exit(1);
                }
                tmp_pkt_queue[queue_id][cur_round]._queue = pkt_queue[queue_id];
                tmp_pkt_queue[queue_id][cur_round].round = cur_round;
                pthread_mutex_unlock(&pkt_queue_mutex[queue_id][cur_round]);
            }
        }
        initTM();
        for (unsigned int i=0; i<NUM_HOSTS; i++) {
            for (unsigned int j=0; j<NUM_HOSTS; j++) {
                if (i==j) continue;
                int queue_id = host_to_queueid[i][j];
                pthread_mutex_unlock(&queue_mutex[queue_id]);
            }
        }

	    init_sols();
        // setup the demand
        mset (&s.future, tmp_TM);

        //call solstice
        sols_schedule(&s);
        sols_check(&s);
        
        //printTM(tmp_TM);
        //call solstice
        // 
        double circuit_bytes[NUM_HOSTS][NUM_HOSTS];
        for (int i = 0; i < s.nday; i++) {
            sols_day_t *day;
            int src, dest;

            day = &s.sched[i];
            //fprintf(fp, "day #%d: T=" FMT_U64 "\n", i, day->len);
            double bytes = (day->len - s.night_len) * circuit_bw; //substract the configuration time (20 usec)

            for (unsigned int dest = 0; dest < NUM_HOSTS; dest++) {
                src = day->input_ports[dest];
                assert(src >= 0);
                if (day->is_dummy[dest]) {
                    //fprintf(fp, "  (%d -> %d)\n", src, dest);
                    continue;
                } else {
                    circuit_bytes[src][dest] += bytes;
                    //fprintf(fp, "%f\n",circuit_bytes[src][dest]);
                    //fprintf(fp, "  %d -> %d\n", src, dest);
                }
            }
        }

        for (unsigned int i=0; i<NUM_HOSTS; i++) {
            for (unsigned int j=0; j<NUM_HOSTS; j++) {
		if (i==j) continue;
                //current_path[i][j] = 0;
                if (circuit_bytes[i][j] >= 1.0) {
                    double rate = circuit_bytes[i][j]*8.0/1000000.0/((s.week_len-(s.night_len*s.nday))/1000000.0);
                    //printf("%f %f\n",circuit_bytes[i][j], circuit_rate[i][j]);
                    //fprintf(fp,"%d -> %d: %f %f Mbps\n",i, j, circuit_bytes[i][j], circuit_rate[i][j]);
                    if (circuit_rate[i][j] != rate){
                        setTC(circuit_cls[i][j], rate);
                        circuit_rate[i][j] = rate;
                    }
                    if (current_path[i][j] != 1) {
                        setPath(i, j, circuit_cls[i][j]);
                        current_path[i][j] = 1;
                    }
                    int queue_id = host_to_queueid[i][j];
                    tmp_pkt_queue[queue_id][cur_round].circuit_bytes = circuit_bytes[i][j];
                }
            }
        }
        //fprintf(fp, "\n\n");
        //transmit
        int rc;
        for (unsigned int i=1; i<=NUM_THREADS; i++) {
            if (tmp_pkt_queue[i][cur_round]._queue.size() == 0) continue;
            p.push(xmitThread, &tmp_pkt_queue[i][cur_round]);
        }
    }
    pthread_exit(NULL);
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
	exit(1);
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

    if (!packet_flag) {
        while ((rv = recv(fd, buf, sizeof(buf), 0))) {
            if (rv < 0)
                continue;
            char* pkt = (char*)malloc(rv);
            memcpy(pkt, buf, rv);

            pthread_mutex_lock(&queue_mutex[tid]);
            traffic_matrix[host_pair[tid].first][host_pair[tid].second] += (rv-88); //only payload size
            traffic_matrix_pkt[host_pair[tid].first][host_pair[tid].second] ++; //only payload size
            pkt_queue[tid].push(std::make_pair(pkt, rv));
            pthread_mutex_unlock(&queue_mutex[tid]);
        }
    } else {
        while ((rv = recv(fd, buf, sizeof(buf), 0))) {
            if (rv < 0)
                continue;
            //char* pkt = (char*)malloc(rv);
            //memcpy(pkt, buf, rv);
            nfq_handle_packet(h[tid], buf, rv);
        }
    }

    printf("unbinding from queue Thread: %ld  \n", tid);
    nfq_destroy_queue(qh);

    printf("closing library handle\n");
    nfq_close(h[tid]);
    pthread_exit(NULL);

    return NULL;

}

void init_sols() {

    sols_init(&s, NUM_HOSTS); /* init for 8 hosts */
    s.night_len = 20; //20usec
    s.week_len = 3000; //3msec
    s.avg_day_len = 40;
    s.min_day_len = 40;
    s.day_len_align = 10;
    s.link_bw = circuit_bw;
    s.pack_bw = packet_bw;
}

void init() {

    int read;
    char *line = NULL;
    size_t len = 0; 
    FILE *f_host = fopen("/home/ec2-user/hosts.txt","r");
    while ((read=getline(&line, &len, f_host))!=-1) {
        char host[20];
        sscanf(line,"%s", host);
        host_list.push_back(std::string(host));
    } 
    fclose(f_host);

    fp = fopen("./TM.dat","a");
    
    NUM_HOSTS = host_list.size();

    for (unsigned int i=0; i<NUM_HOSTS; i++) {
        for (unsigned int j=0; j<NUM_HOSTS; j++) {
            circuit_rate[i][j] = 100.0;
            current_path[i][j] = 0;
        }
    }
    
    initTM();
    initTC();
    initIPT();
    initPath();

}

void 
intHandler(int signum) {
    //destroy all threads
    //pthread_exit(NULL);
    //
    /*uint32_t prev = 0;
    for (unsigned int i=0; i<ids[3].size(); i++){
        if (ids[3][i] != prev+1)
            printf("something is wrong %d %d\n",ids[3][i], prev);
        prev = ids[3][i];
    }*/

    clearIPT();
    clearTC();
    fclose(fp);
    exit(0);
}


int main(int argc, char *argv[]) {

    signal(SIGINT, intHandler);

    if (argc > 1) {
        if ( strcmp(argv[1], "packet") == 0) {
            packet_flag = 1;
            printf("Run as a packet switch\n");
        }
    }
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

    if (!packet_flag) {
        sleep(1);
        printf("In main: creating scheduler thread\n");
        rc = pthread_create(&sched_thread, NULL, SchedThread, (void *) 0);

        if (rc) {
            printf("ERROR; return code from pthread_create() is %d\n", rc);
            exit(-1);
        }
    }

    while (1) {
        sleep(10);
    }

}
