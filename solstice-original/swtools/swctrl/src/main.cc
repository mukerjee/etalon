#include "swctrl/swctrl.hh"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cassert>

#include <arpa/inet.h>
#include <linux/if_packet.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/ether.h>
#include <unistd.h>

static void hprint(void *buf, size_t n) {
    uint8_t *p = (uint8_t *)(buf);
    for (size_t i = 0; i < n; i++) {
        printf("%02x", p[i]);
        if ((i + 1) % 16 == 0) printf("\n");
        else if ((i + 1) % 8 == 0) printf("  ");
        else if ((i + 1) % 4 == 0) printf(" ");
    }
    
    if (n % 16 != 0) printf("\n");
}

static void panic() {
    exit(-1);
}

static const char * iface_r7 = "m55";
static const char * iface_other = "lo";

static const char * smart_iface() {
    static const size_t LEN = 1024;
    char buf[LEN];
    int ret = gethostname(buf, LEN);
    if (ret != 0) {
        perror("gethostname");
    }

    if (strncmp(buf, "reactor7", 8) == 0) {
        return iface_r7;
    }
    
    return iface_other;
}

struct sender_t {
    int sockfd;
    struct sockaddr_ll addr;
    uint8_t buf[ETH_FRAME_LEN];
    bool verbose;

    void init(const char * ifname) {
        sockfd = socket(AF_PACKET, SOCK_RAW, IPPROTO_RAW);
        if (sockfd == -1) {
            perror("socket");
            panic();
        }

        struct ifreq if_idx;
        strncpy(if_idx.ifr_name, ifname, IFNAMSIZ - 1);
        if (ioctl(sockfd, SIOCGIFINDEX, &if_idx) < 0) {
            perror("if_idx");
            panic();
        }

        addr.sll_ifindex = if_idx.ifr_ifindex;
        addr.sll_halen = ETH_ALEN;
        memset(addr.sll_addr, 0xFF, ETH_ALEN);
    }
    
    void send(void *p, size_t n) {
        if (verbose) {
            printf("[send]\n");
            hprint(p, n);
        }

        if (sendto(sockfd, p, n, 0, 
                (struct sockaddr*) &addr,
                sizeof(struct sockaddr_ll)) < 0) {
            perror("sendto");
            panic();
        }
    }

    size_t recv(uint8_t ** p) {
        int n = recvfrom(sockfd, buf, ETH_FRAME_LEN, 0, NULL, NULL);
        if (n < 0) {
            perror("recvfrom");
            panic();
        }

        if (p != NULL) {
            *p = buf;
        }
        
        if (verbose) {
            printf("[recv]\n");
            hprint(buf, n);
        }

        return size_t(n);
    }
};

static sender_t sender;
static void ctrl_send(void *p, size_t n) { sender.send(p, n); }

uint64_t s(double n) { return uint64_t(n / 6.4e-9); }
uint64_t ms(double n) { return uint64_t(n / 6.4e-6); }
uint64_t us(double n) { return uint64_t(n / 6.4e-3); }

uint8_t hex4(char c) {
    if (c >= '0' && c <= '9') return uint8_t(c - '0');
    if (c >= 'a' && c <= 'f') return uint8_t(c - 'a' + 10);
    if (c >= 'A' && c <= 'F') return uint8_t(c - 'a' + 10);
    assert(false);
    return 0;
}

uint8_t hex8(const char * s) {
    return (hex4(s[0]) << 4) + hex4(s[1]);
}

void parse_addr(const char * s, uint8_t * addr) {
    for (size_t i = 0; i < 6; i++) {
        char sep = s[i * 3 + 2];
        if (i == 5) { 
            assert(sep == '\0'); 
        } else {
            assert(sep == ':' || sep == '-');
        }
        
        addr[i] = hex8(&s[i * 3]);
    }
}

const char * ADDR_I95 = "90:e2:ba:2c:41:95";
const char * ADDR_I94 = "90:e2:ba:2c:41:94";
const char * ADDR_I40 = "90:e2:ba:2c:3e:40";
const char * ADDR_I41 = "90:e2:ba:2c:3e:41";
const char * ADDR_I98 = "90:e2:ba:2f:fc:98";
const char * ADDR_I99 = "90:e2:ba:2f:fc:99";
const char * ADDR_I7C = "90:e2:ba:30:01:7c";
const char * ADDR_I7D = "90:e2:ba:30:01:7d";
const char * ADDR_I0C = "90:e2:ba:2f:fa:0c";
const char * ADDR_I0D = "90:e2:ba:2f:fa:0d";
const char * ADDR_I8C = "90:e2:ba:2f:fc:8c";
const char * ADDR_I8D = "90:e2:ba:2f:fc:8d";

int main(int argc, char ** argv) {
    const char * hostname = NULL; 
    if (argc >= 2) {
        hostname = argv[1];
    } else {
        hostname = smart_iface();
    }

    // setup the sender
    sender.verbose = true;
    printf("using %s\n", hostname);
    sender.init(hostname);
    ctrl_set_sender(ctrl_send);

    // ctrl_reset(); // reset the entire chip
    // usleep(1000000);
    
    sw_init(); // initialize the switch

    ctrl_timings_t timings; {
        timings.t_night = us(20);
        timings.t_plan = us(40);
        timings.t_map = us(30);
        timings.t_sync = us(20);
        timings.t_switch = us(10);
    }
    ctrl_set_timings(&timings);
    
    uint8_t addrs[NHOST][MAC_ADDR_SIZE];
    parse_addr(ADDR_I95, addrs[0]);
    parse_addr(ADDR_I41, addrs[1]);
    parse_addr(ADDR_I99, addrs[2]);
    parse_addr(ADDR_I7D, addrs[3]);
    bzero(addrs[4], MAC_ADDR_SIZE);
    bzero(addrs[5], MAC_ADDR_SIZE);
    bzero(addrs[6], MAC_ADDR_SIZE);
    bzero(addrs[7], MAC_ADDR_SIZE);
    ctrl_set_addr(addrs);

    ctrl_reset_sched(); // reset the schedule buffer table
    
    // config linerate senders on port b3 and f3
    /*
    sender_conf_t sender_b3 = { enable:false, len:8, pad:0 }; 
    sender_conf_t sender_f3 = { enable:false, len:8, pad:0 }; 
    ctrl_set_linerates(&sender_b3, &sender_f3);
    */

    static const size_t MAX_NDAY = 255;
    sched_t sched[MAX_NDAY];

    static const uint8_t NDAY = 2;
    uint64_t t = uint64_t(.5 / 6.4e-6);
    uint64_t t_weeksig = t * NDAY - uint64_t(.4 / 6.4e-6);
    
    // build the schedule for the first day
    /*
    mapping_t map1[] = { {0, 1}, {1, 2}, {2, 0} };
    sched_build(&sched[0], t, map1, sizeof(map1) / sizeof(mapping_t));

    // build the schedule for the second day
    mapping_t map2[] = { {0, 2}, {1, 0}, {2, 1} };
    sched_build(&sched[1], t, map2, sizeof(map2) / sizeof(mapping_t));
    */

    ctrl_set_weeksig_pos(t_weeksig);
    ctrl_set_sched(sched, NDAY); // write the schedule to FPGA buffer

    ctrl_commit_sched(NDAY); // use the new schedule from last 

    return 0;
}
