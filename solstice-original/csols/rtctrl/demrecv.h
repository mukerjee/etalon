#define DEMAND_PORT 9001

struct DemRecv {
    uint64_t dem[NLANE];
    int last_id[NHOST];
    uint8_t last_src;

    DemRecv() {
        reset();
    }

    void reset() {
        for (int i = 0; i < NHOST; i++) {
            last_id[i] = -1;
        }
        memset(dem, 0, sizeof(uint64_t) * NLANE);
    }

    // return 0 on succ, -1 on failed
    int tryParse(uint8_t *buf, size_t n, bool debug=false) {
        static const size_t UDP_HEADER_LEN = 8;
        static const size_t DEMAND_HEADER_LEN = 3;

        if (n < 14) {
            return -1;
        }

        uint8_t vlan = buf[12];
        size_t eth_hdr_len;
        int i;
        struct iphdr *iph;
        struct udphdr *udph;

        /* check the packet for vlan tag, extract the ip header, check port
         * matches demand estimation port extract the source,
         * destination(s) and value for demand estimation save the value in
         * demand matrix
         */

        // hexadecimal 0x81 for vlan
        if (vlan == 0x81) {
            eth_hdr_len = 18;
        } else {
            eth_hdr_len = 14;
        }

        if (n < eth_hdr_len) {
            return -1;
        }

        uint8_t packet_type = buf[eth_hdr_len-2];
        if (packet_type != 0x08) { // not an ethernet packet
            return -1;
        }
        buf += eth_hdr_len; // skip the ethernet hdr
        n -= eth_hdr_len;

        if (n < 4) {
            return -1;
        }
        iph = (struct iphdr *)(buf);
        size_t iph_len = size_t(iph->ihl) << 2;
        if (iph->protocol != IPPROTO_UDP) { // not a udp packet
            return -1;
        }
        if (n < iph_len) {
            return -1;
        }
        buf += iph_len; // skip the ip hdr
        n -= eth_hdr_len;

        if (n < UDP_HEADER_LEN) {
            return -1;
        }
        udph = (struct udphdr *)(buf);
        int port = ntohs(udph->source);
        if (port != DEMAND_PORT) { // not the right udp port
            return -1;
        }
        buf += UDP_HEADER_LEN;
        n -= UDP_HEADER_LEN;

        if (n < DEMAND_HEADER_LEN) {
            return -1;
        }
        uint8_t packet_id = buf[0];

        uint8_t count = buf[1];
        uint8_t src = buf[2];
        if (src >= NHOST) {
            return -1;
        }

        uint8_t last = last_id[src];
        if (last >= 0 && (uint8_t)(last + 1) != packet_id) {
            fprintf(stderr, "demrecv id jump %d->%d\n", last, packet_id);
        }
        last_id[src] = packet_id;

        last_src = src;
        buf += DEMAND_HEADER_LEN;
        n -= DEMAND_HEADER_LEN;

        static const size_t ENTRY_SIZE = 1 + sizeof(uint32_t);
        if (n < count * ENTRY_SIZE) {
            return -1;
        }

        for (i = 0; i < count; i++) {
            uint8_t host = buf[0];
            uint32_t value;
            memcpy(&value, &buf[1], sizeof(uint32_t));
            value = ntohl(value) * 1024; // now in bytes
            dem[src * NHOST + host] = uint64_t(value);

            buf += ENTRY_SIZE;
            n -= ENTRY_SIZE;
            assert(n >= 0);
        }

        return 0;
    }

    uint8_t id() {
        return last_id[last_src];
    }
};
