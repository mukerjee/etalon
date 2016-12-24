struct __attribute__ ((__packed__)) ethhdr_t {
    uint8_t dest[MAC_ADDR_SIZE];
    uint8_t src[MAC_ADDR_SIZE];
    uint16_t type;
};

const char * CONTROL_SRCIP = "192.168.1.40";
const char * CONTROL_DESTIP = "192.168.1.255";
const uint16_t CTRL_PORT = 8203;
const uint8_t CTRL_MAC[MAC_ADDR_SIZE] = {0x90, 0xe2, 0xba, 0x2f, 0x8e, 0xac};

inline uint16_t ip_fast_csum(uint8_t * iph, unsigned int ihl) {
    unsigned int sum;

    __asm__ __volatile__(
        "movl (%1), %0      ;\n"
        "subl $4, %2        ;\n"
        "jbe 2f             ;\n"
        "addl 4(%1), %0     ;\n"
        "adcl 8(%1), %0     ;\n"
        "adcl 12(%1), %0    ;\n"
        "1:      adcl 16(%1), %0     ;\n"
        "lea 4(%1), %1      ;\n"
        "decl %2            ;\n"
        "jne 1b             ;\n"
        "adcl $0, %0        ;\n"
        "movl %0, %2        ;\n"
        "shrl $16, %0       ;\n"
        "addw %w2, %w0      ;\n"
        "adcl $0, %0        ;\n"
        "notl %0            ;\n"
        "2:                         ;\n"
        /*  Since the input registers which are loaded with iph and ipl
            are modified, we must also specify them as outputs, or gcc
            will assume they contain their original values. */
        : "=r" (sum), "=r" (iph), "=r" (ihl)
        : "1" (iph), "2" (ihl)
        : "memory");

    return uint16_t(sum);
}

void fillIpCsum(struct iphdr *ip) {
    ip->check = 0;

    uint64_t sum = 0;
    uint16_t *p = (uint16_t *)(ip);
    for (int i = 0; i < 10; i++) {
        sum += *p++;
    }
    // fold, truncate, revert and fill it in
    ip->check = ~(uint16_t(sum + (sum >> 16)));
}

struct UdpSender {
    Packer packer;
    struct ethhdr_t eth;
    struct iphdr ip;
    struct udphdr udp;

    UdpSender() {
        init();
    }

    void init() {
        assert(sizeof(eth) == 14);
        assert(sizeof(ip) == 20);
        assert(sizeof(udp) == 8);

        memset(eth.dest, 0xff, MAC_ADDR_SIZE);
        memcpy(eth.src, CTRL_MAC, MAC_ADDR_SIZE);
        eth.type = htons(0x0800);

        ip.ihl = 5;
        ip.version = 4;
        ip.tos = 0x20;
        ip.tot_len = 0; // to be determined
        ip.id = htons(7337);
        ip.frag_off = 0;
        ip.ttl = 255;
        ip.protocol = 0x11; // udp
        ip.check = 0; // to be calculated

        // hard coded addresses
        ip.saddr = inet_addr(CONTROL_SRCIP);
        ip.daddr = inet_addr(CONTROL_DESTIP);

        // hard coded ports
        udp.source = htons(CTRL_PORT);
        udp.dest = htons(CTRL_PORT);
        udp.len = 0;
        udp.check = 0;
    }

    Packer * prepare(size_t nbytes) {
        ip.tot_len = htons(sizeof(ip) + sizeof(udp) + nbytes);
        udp.len = htons(sizeof(udp) + nbytes);
        fillIpCsum(&ip);
        // ip.check = 0;
        // ip_fast_csum((uint8_t *)(&ip), 5); // probably too fancy...

        packer.clear();
        packer.pack(&eth, sizeof(eth));
        packer.pack(&ip, sizeof(ip));
        packer.pack(&udp, sizeof(udp));

        return &packer;
    }
};
