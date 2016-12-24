#include "swctrl.hh"
#include <cassert>
#include <cstring>
#include <unistd.h>
#include <cstdio>

static inline uint64_t u64(uint8_t * c) {
    uint64_t ret = 0;

    ret |= uint64_t(c[0]);
    ret |= (uint64_t(c[1]) << 8);
    ret |= (uint64_t(c[2]) << 16);
    ret |= (uint64_t(c[3]) << 24);
    ret |= (uint64_t(c[4]) << 32);
    ret |= (uint64_t(c[5]) << 40);
    ret |= (uint64_t(c[6]) << 48);
    ret |= (uint64_t(c[7]) << 56);

    return ret;
}

static inline uint32_t u32(uint8_t * c) {
    uint32_t ret = 0;

    ret |= uint32_t(c[0]);
    ret |= (uint32_t(c[1]) << 8);
    ret |= (uint32_t(c[2]) << 16);
    ret |= (uint32_t(c[3]) << 24);

    return ret;
}

/*
static inline uint16_t u16(uint8_t * c) { 
    return uint16_t(c[0]) | (uint16_t(c[1]) << 8);
}
*/

// static inline uint16_t u8(uint8_t * c) { return c[0]; }

enum {
    CMD_UNKNOWN = 0x00,

    CMD_SCHED_SET = 0x80,
    CMD_SCHED_SWAP = 0x81,

    CMD_RESET = 0x90,
    CMD_ADDR = 0x91,
    CMD_FLAGS = 0x92,
    CMD_WEEKSIG = 0x93,
    CMD_TIMINGS = 0x94,
    CMD_LANE_SEL = 0x95,
    CMD_STAT = 0x96,
    CMD_SENDER = 0x9f,
    CMD_LOOPBACK = 0x97,

    CMD_DEBUG_SEL = 0xDB,
};

static const size_t MIN_PACKLEN = 60;

bool is_weeksig(void *content, size_t size, uint32_t * nweek) {
    uint8_t *p = (uint8_t *)(content);
    assert(size >= MIN_PACKLEN);

    bool ret = (p[14] == 0x1E && p[15] == 0xEC);
    if (ret && nweek != NULL) {
        *nweek = u32(&p[16 + 16 + 4]);
    }

    return ret;
}

static void (*ctrl_send)(void *, size_t) = NULL;

void ctrl_set_sender(void (*sender)(void *, size_t)) {
    ctrl_send = sender;
}

static const uint8_t ROUTE_CTRL = 0xFC;
static const uint8_t ROUTE_RESET = 0xE1;
static const uint8_t ROUTE_SW = 0xE2;

static const uint8_t * MAC_BC = (uint8_t *) (
        "\xff\xff\xff\xff\xff\xff");
static const uint8_t * IP_TYPE = (uint8_t *) ("\x08\x00");

inline static void hprint(void *buf, size_t n) {
    uint8_t *p = (uint8_t *)(buf);

    for (size_t i = 0; i < n; i++) {
        printf("%02x", p[i]);
        if ((i + 1) % 16 == 0) 
            printf("\n");
        else if ((i + 1) % 8 == 0) 
            printf("  ");
        else if ((i + 1) % 4 == 0) 
            printf(" ");
    }

    if (n % 16 != 0) {
        printf("\n");
    }
}

struct packer_t {
    static const size_t MAX_SIZE = 1500;

    uint8_t buf[MAX_SIZE];
    size_t size;

    uint8_t * p() { return &buf[size]; }
    void clear() { size = 0; }
    bool have(size_t n) { return size + n <= MAX_SIZE; }

    void pad(size_t n) {
        assert(have(n));
        bzero(p(), n);
        size += n;
    }

    void u64(uint64_t i) {
        assert(have(8));

        uint8_t * pt = p();
        pt[0] = uint8_t(i);
        pt[1] = uint8_t(i >> 8);
        pt[2] = uint8_t(i >> 16);
        pt[3] = uint8_t(i >> 24);

        pt[4] = uint8_t(i >> 32);
        pt[5] = uint8_t(i >> 40);
        pt[6] = uint8_t(i >> 48);
        pt[7] = uint8_t(i >> 56);

        size += 8;
    }

    void u32(uint32_t i) {
        assert(have(4));

        uint8_t * pt = p();
        pt[0] = uint8_t(i);
        pt[1] = uint8_t(i >> 8);
        pt[2] = uint8_t(i >> 16);
        pt[3] = uint8_t(i >> 24);

        size += 4;
    }

    void u16(uint16_t i) {
        assert(have(2));

        uint8_t * pt = p();
        pt[0] = uint8_t(i);
        pt[1] = uint8_t(i >> 8);

        size += 2;
    }

    void c(uint8_t b) { u8(b); }
    void u8(uint8_t i) {
        assert(have(1));

        *(p()) = i;
        size++;
    }

    void str(const uint8_t * s, size_t n) {
        assert(have(n));

        memcpy(p(), s, n);
        size += n;
    }

    void send() {
        if (size < MIN_PACKLEN) {
            pad(MIN_PACKLEN - size);
        }
        
        if (ctrl_send != NULL) {
            ctrl_send((void *)(buf), size);
        }
    }

    void start(uint8_t route=ROUTE_CTRL, uint8_t extra=0) {
        clear();
        str(MAC_BC, MAC_ADDR_SIZE);
        c(route);
        c(extra);
        pad(MAC_ADDR_SIZE - 2);
        str(IP_TYPE, 2);
    }

    void start_cmd(uint8_t cmd, uint8_t extra=0) {
        start();
        c(cmd);
        c(extra);
    }

    void sender_conf(sender_conf_t * s) {
        u8(s->enable ? 1 : 0);
        pad(1);
        u16(s->len);
        u32(s->pad);
    }

    void print() {
        printf("(size=%lu)\n", size);
        hprint(buf, size);
    }
};

static packer_t packer;

void ctrl_reset() {
    packer.start(ROUTE_RESET);
    packer.send();
}

void ctrl_reset_sched() {
    packer.start();
    packer.c(CMD_RESET);
    packer.send();
}

void ctrl_set_addr(uint8_t addrs[NHOST][MAC_ADDR_SIZE]) {
    packer.start_cmd(CMD_ADDR);

    for (size_t i = 0; i < NHOST; i++) {
        packer.str(addrs[i], MAC_ADDR_SIZE);
        packer.pad(2);
    }
    // packer.pad(8);

    packer.send();
}

static uint16_t build_flag(ctrl_flags_t *flags) {
    uint16_t f = 0;
    if (!flags->send_pfc) f |= 0x1;
    if (!flags->send_weekend) f |= 0x2;
    if (!flags->send_weeksig) f |= 0x4;
    if (!flags->sync_internal) f |= 0x8;
    if (flags->dpdk_frame_type) f |= 0x10;

    return f;
}

void ctrl_set_flags(ctrl_flags_t * flags, ctrl_flags_t *slave_flags) {
    packer.start_cmd(CMD_FLAGS);

    uint16_t f = build_flag(flags);
    packer.c((uint8_t)(f & 0xff));
    packer.c((uint8_t)((f >> 8) & 0xff));
    packer.pad(2);

    if (slave_flags != NULL) { 
        f = build_flag(slave_flags); 
    }
    packer.c(f);
    packer.pad(3);

    packer.send();
}

void ctrl_config_senders(sender_conf_t * b3, sender_conf_t * f3) {
    packer.start_cmd(CMD_SENDER);
    packer.sender_conf(b3);
    packer.sender_conf(f3);
    packer.send();
}

void ctrl_set_weeksig_pos(uint64_t pos) {
    packer.start_cmd(CMD_WEEKSIG);
    packer.u64(pos);
    packer.send();
}

void ctrl_set_timings(ctrl_timings_t * timings) {
    packer.start_cmd(CMD_TIMINGS);

    packer.u64(timings->t_night);
    packer.u64(timings->t_plan);
    packer.u64(timings->t_map);
    packer.u64(timings->t_sync);
    packer.u64(timings->t_switch);

    packer.send();
}

void ctrl_set_sched(sched_t * s, uint8_t n, uint8_t offset) {
    assert(n <= 16);

    packer.start_cmd(CMD_SCHED_SET, offset);

    for (size_t i = 0; i < n; i++) {
        assert(offset + n < 0xff);

        packer.u64(s[i].t);
        packer.str(s[i].pfc_day, size_t(NHOST));
        packer.str(s[i].pfc_night, size_t(NHOST));
        packer.str(s[i].circwl, size_t(NHOST));
        packer.str(s[i].packbl, size_t(NHOST));
        packer.str(s[i].portmap, size_t(NPORT));
    }

    packer.send();
}

void ctrl_commit_sched(uint8_t nday, bool load_start) {
    packer.start_cmd(CMD_SCHED_SWAP, nday);
    if (load_start) {
        packer.u64(1);
    } else {
        packer.u64(0);
    }

    packer.send();
}

// mordia related settings
// SE_IN_D0 = 39, SE_IN_D1 = 40, SE_IN_D2 = 41, SE_IN_D3 = 42,
// SE_OUT_D0 = 3, SE_OUT_D1 = 2, SE_OUT_D2 = 1, SE_OUT_D3 = 0,
enum {
    // SE
    SE_IN_D0 = 23, SE_IN_D1 = 21, SE_IN_D2 = 22, SE_IN_D3 = 20,
    SE_IN_E0 = 12, SE_IN_E1 = 14, SE_IN_E2 = 13, SE_IN_E3 = 15,
    SE_IN_F0 = 29, SE_IN_F1 = 28, SE_IN_F2 = 31, SE_IN_F3 = 30,

    SE_OUT_D0 = 47, SE_OUT_D1 = 45, SE_OUT_D2 = 46, SE_OUT_D3 = 44,
    SE_OUT_E0 = 36, SE_OUT_E1 = 38, SE_OUT_E2 = 37, SE_OUT_E3 = 39,
    SE_OUT_F0 = 5, SE_OUT_F1 = 4, SE_OUT_F2 = 7, SE_OUT_F3 = 6,

    // SW
    SW_IN_D0 = 47, SW_IN_D1 = 45, SW_IN_D2 = 46, SW_IN_D3 = 44,
    SW_IN_E0 = 36, SW_IN_E1 = 38, SW_IN_E2 = 37, SW_IN_E3 = 39,
    SW_IN_F0 = 5, SW_IN_F1 = 4, SW_IN_F2 = 7, SW_IN_F3 = 6,

    SW_OUT_D0 = 23, SW_OUT_D1 = 21, SW_OUT_D2 = 22, SW_OUT_D3 = 20,
    SW_OUT_E0 = 12, SW_OUT_E1 = 14, SW_OUT_E2 = 13, SW_OUT_E3 = 15,
    SW_OUT_F0 = 29, SW_OUT_F1 = 28, SW_OUT_F2 = 31, SW_OUT_F3 = 30,

    // NE
    NE_IN_D0 = 35, NE_IN_D1 = 33, NE_IN_D2 = 34, NE_IN_D3 = 32,
    NE_IN_E0 = 40, NE_IN_E1 = 42, NE_IN_E2 = 41, NE_IN_E3 = 43,
    NE_IN_F0 = 17, NE_IN_F1 = 16, NE_IN_F2 = 19, NE_IN_F3 = 18,

    NE_OUT_D0 = 11, NE_OUT_D1 = 9, NE_OUT_D2 = 10, NE_OUT_D3 = 8,
    NE_OUT_E0 = 0, NE_OUT_E1 = 2, NE_OUT_E2 = 1, NE_OUT_E3 = 3,
    NE_OUT_F0 = 25, NE_OUT_F1 = 24, NE_OUT_F2 = 27, NE_OUT_F3 = 26,

    // NW
    NW_IN_D0 = 11, NW_IN_D1 = 9, NW_IN_D2 = 10, NW_IN_D3 = 8,
    NW_IN_E0 = 0, NW_IN_E1 = 2, NW_IN_E2 = 1, NW_IN_E3 = 3,
    NW_IN_F0 = 25, NW_IN_F1 = 24, NW_IN_F2 = 27, NW_IN_F3 = 26,

    NW_OUT_D0 = 35, NW_OUT_D1 = 33, NW_OUT_D2 = 34, NW_OUT_D3 = 32,
    NW_OUT_E0 = 40, NW_OUT_E1 = 42, NW_OUT_E2 = 41, NW_OUT_E3 = 43,
    NW_OUT_F0 = 17, NW_OUT_F1 = 16, NW_OUT_F2 = 19, NW_OUT_F3 = 18,

    // MORDIA1
    M1_IN_D0 = 39, M1_IN_D1 = 40, M1_IN_D2 = 41, M1_IN_D3 = 42,
    M1_IN_E0 = 39, M1_IN_E1 = 40, M1_IN_E2 = 41, M1_IN_E3 = 42,
    M1_IN_F0 = 39, M1_IN_F1 = 40, M1_IN_F2 = 41, M1_IN_F3 = 42,

    M1_OUT_D0 = 3, M1_OUT_D1 = 2, M1_OUT_D2 = 1, M1_OUT_D3 = 0,
    M1_OUT_E0 = 3, M1_OUT_E1 = 2, M1_OUT_E2 = 1, M1_OUT_E3 = 0,
    M1_OUT_F0 = 3, M1_OUT_F1 = 2, M1_OUT_F2 = 1, M1_OUT_F3 = 0,

    // MORDIA2
    M2_IN_D0 = 55, M2_IN_D1 = 56, M2_IN_D2 = 57, M2_IN_D3 = 58,
    M2_IN_E0 = 55, M2_IN_E1 = 56, M2_IN_E2 = 57, M2_IN_E3 = 58,
    M2_IN_F0 = 55, M2_IN_F1 = 56, M2_IN_F2 = 57, M2_IN_F3 = 58,

    M2_OUT_D0 = 7, M2_OUT_D1 = 6, M2_OUT_D2 = 5, M2_OUT_D3 = 4,
    M2_OUT_E0 = 7, M2_OUT_E1 = 6, M2_OUT_E2 = 5, M2_OUT_E3 = 4,
    M2_OUT_F0 = 7, M2_OUT_F1 = 6, M2_OUT_F2 = 5, M2_OUT_F3 = 4,

    // VIRTUAL1
    V1_IN_D0 = 0, V1_IN_D1 = 1, V1_IN_D2 = 2, V1_IN_D3 = 3,
    V1_IN_E0 = 0, V1_IN_E1 = 1, V1_IN_E2 = 2, V1_IN_E3 = 3,
    V1_IN_F0 = 0, V1_IN_F1 = 1, V1_IN_F2 = 2, V1_IN_F3 = 3,

    V1_OUT_D0 = 0, V1_OUT_D1 = 1, V1_OUT_D2 = 2, V1_OUT_D3 = 3,
    V1_OUT_E0 = 0, V1_OUT_E1 = 1, V1_OUT_E2 = 2, V1_OUT_E3 = 3,
    V1_OUT_F0 = 0, V1_OUT_F1 = 1, V1_OUT_F2 = 2, V1_OUT_F3 = 3,

    // VIRTUAL2
    V2_IN_D0 = 4, V2_IN_D1 = 5, V2_IN_D2 = 6, V2_IN_D3 = 7,
    V2_IN_E0 = 4, V2_IN_E1 = 5, V2_IN_E2 = 6, V2_IN_E3 = 7,
    V2_IN_F0 = 4, V2_IN_F1 = 5, V2_IN_F2 = 6, V2_IN_F3 = 7,

    V2_OUT_D0 = 4, V2_OUT_D1 = 5, V2_OUT_D2 = 6, V2_OUT_D3 = 7,
    V2_OUT_E0 = 4, V2_OUT_E1 = 5, V2_OUT_E2 = 6, V2_OUT_E3 = 7,
    V2_OUT_F0 = 4, V2_OUT_F1 = 5, V2_OUT_F2 = 6, V2_OUT_F3 = 7,
};

static uint8_t LANE_NAME[] = {
    0xd0, 0xd1, 0xd2, 0xd3,
    0xe0, 0xe1, 0xe2, 0xe3,
    0xf0, 0xf1, 0xf2, 0xf3,
};

static uint8_t SE_INS[] = {
    SE_IN_D0, SE_IN_D1, SE_IN_D2, SE_IN_D3,
    SE_IN_E0, SE_IN_E1, SE_IN_E2, SE_IN_E3,
    SE_IN_F0, SE_IN_F1, SE_IN_F2, SE_IN_F3,
};
static uint8_t SW_INS[] = {
    SW_IN_D0, SW_IN_D1, SW_IN_D2, SW_IN_D3,
    SW_IN_E0, SW_IN_E1, SW_IN_E2, SW_IN_E3,
    SW_IN_F0, SW_IN_F1, SW_IN_F2, SW_IN_F3,
};
static uint8_t NE_INS[] = {
    NE_IN_D0, NE_IN_D1, NE_IN_D2, NE_IN_D3,
    NE_IN_E0, NE_IN_E1, NE_IN_E2, NE_IN_E3,
    NE_IN_F0, NE_IN_F1, NE_IN_F2, NE_IN_F3,
};
static uint8_t NW_INS[] = {
    NW_IN_D0, NW_IN_D1, NW_IN_D2, NW_IN_D3,
    NW_IN_E0, NW_IN_E1, NW_IN_E2, NW_IN_E3,
    NW_IN_F0, NW_IN_F1, NW_IN_F2, NW_IN_F3,
};
static uint8_t M1_INS[] = {
    M1_IN_D0, M1_IN_D1, M1_IN_D2, M1_IN_D3,
    M1_IN_E0, M1_IN_E1, M1_IN_E2, M1_IN_E3,
    M1_IN_F0, M1_IN_F1, M1_IN_F2, M1_IN_F3,
};
static uint8_t M2_INS[] = {
    M2_IN_D0, M2_IN_D1, M2_IN_D2, M2_IN_D3,
    M2_IN_E0, M2_IN_E1, M2_IN_E2, M2_IN_E3,
    M2_IN_F0, M2_IN_F1, M2_IN_F2, M2_IN_F3,
};
static uint8_t V1_INS[] = {
    V1_IN_D0, V1_IN_D1, V1_IN_D2, V1_IN_D3,
    V1_IN_E0, V1_IN_E1, V1_IN_E2, V1_IN_E3,
    V1_IN_F0, V1_IN_F1, V1_IN_F2, V1_IN_F3,
};
static uint8_t V2_INS[] = {
    V2_IN_D0, V2_IN_D1, V2_IN_D2, V2_IN_D3,
    V2_IN_E0, V2_IN_E1, V2_IN_E2, V2_IN_E3,
    V2_IN_F0, V2_IN_F1, V2_IN_F2, V2_IN_F3,
};

static uint8_t SE_OUTS[] = {
    SE_OUT_D0, SE_OUT_D1, SE_OUT_D2, SE_OUT_D3,
    SE_OUT_E0, SE_OUT_E1, SE_OUT_E2, SE_OUT_E3,
    SE_OUT_F0, SE_OUT_F1, SE_OUT_F2, SE_OUT_F3,
};
static uint8_t SW_OUTS[] = {
    SW_OUT_D0, SW_OUT_D1, SW_OUT_D2, SW_OUT_D3,
    SW_OUT_E0, SW_OUT_E1, SW_OUT_E2, SW_OUT_E3,
    SW_OUT_F0, SW_OUT_F1, SW_OUT_F2, SW_OUT_F3,
};
static uint8_t NE_OUTS[] = {
    NE_OUT_D0, NE_OUT_D1, NE_OUT_D2, NE_OUT_D3,
    NE_OUT_E0, NE_OUT_E1, NE_OUT_E2, NE_OUT_E3,
    NE_OUT_F0, NE_OUT_F1, NE_OUT_F2, NE_OUT_F3,
};
static uint8_t NW_OUTS[] = {
    NW_OUT_D0, NW_OUT_D1, NW_OUT_D2, NW_OUT_D3,
    NW_OUT_E0, NW_OUT_E1, NW_OUT_E2, NW_OUT_E3,
    NW_OUT_F0, NW_OUT_F1, NW_OUT_F2, NW_OUT_F3,
};
static uint8_t M1_OUTS[] = {
    M1_OUT_D0, M1_OUT_D1, M1_OUT_D2, M1_OUT_D3,
    M1_OUT_E0, M1_OUT_E1, M1_OUT_E2, M1_OUT_E3,
    M1_OUT_F0, M1_OUT_F1, M1_OUT_F2, M1_OUT_F3,
};
static uint8_t M2_OUTS[] = {
    M2_OUT_D0, M2_OUT_D1, M2_OUT_D2, M2_OUT_D3,
    M2_OUT_E0, M2_OUT_E1, M2_OUT_E2, M2_OUT_E3,
    M2_OUT_F0, M2_OUT_F1, M2_OUT_F2, M2_OUT_F3,
};
static uint8_t V1_OUTS[] = {
    V1_OUT_D0, V1_OUT_D1, V1_OUT_D2, V1_OUT_D3,
    V1_OUT_E0, V1_OUT_E1, V1_OUT_E2, V1_OUT_E3,
    V1_OUT_F0, V1_OUT_F1, V1_OUT_F2, V1_OUT_F3,
};
static uint8_t V2_OUTS[] = {
    V2_OUT_D0, V2_OUT_D1, V2_OUT_D2, V2_OUT_D3,
    V2_OUT_E0, V2_OUT_E1, V2_OUT_E2, V2_OUT_E3,
    V2_OUT_F0, V2_OUT_F1, V2_OUT_F2, V2_OUT_F3,
};

static uint8_t *QUAD_INS[] = { 
    SE_INS, SW_INS, NE_INS, NW_INS, 
    M1_INS, M2_INS, V1_INS, V2_INS };
static uint8_t *QUAD_OUTS[] = { 
    SE_OUTS, SW_OUTS, NE_OUTS, NW_OUTS, 
    M1_OUTS, M2_OUTS, V1_OUTS, V2_OUTS };

static uint8_t port_ins[NHOST] = { 
    SE_IN_F0, SE_IN_F1, SE_IN_F2, SE_IN_F3,
    SW_IN_F0, SW_IN_F1, SW_IN_F2, SW_IN_F3,
};
static uint8_t port_outs[NHOST] = { 
    SE_OUT_F0, SE_OUT_F1, SE_OUT_F2, SE_OUT_F3,
    SW_OUT_F0, SW_OUT_F1, SW_OUT_F2, SW_OUT_F3,
};

uint8_t out_port(uint8_t lane) { return port_outs[lane]; }
uint8_t in_port(uint8_t lane) { return port_ins[lane]; }

static uint64_t tx_vector = 0;
static uint64_t rx_vector = 0;

void ctrl_config_lanes() {
    assert(tx_vector != 0 && rx_vector != 0);

    packer.start_cmd(CMD_LANE_SEL);
    packer.u64(tx_vector);
    packer.u64(rx_vector);

    packer.send();
}

void ctrl_use_lanes(uint8_t master, uint8_t slave, 
        uint8_t *tx, uint8_t *rx) {
    assert(0 <= master && master < 8);
    assert(0 <= slave && slave < 8);
    tx_vector = 0;
    rx_vector = 0;
    
    for (int i = 0; i < 8; i++) {
        int q = (i < 4) ? master : slave;

        port_ins[i] = QUAD_INS[q][tx[i]];
        port_outs[i] = QUAD_OUTS[q][rx[i]];

        tx_vector |= (uint64_t(LANE_NAME[tx[i]])) << (8 * i);
        rx_vector |= (uint64_t(LANE_NAME[rx[i]])) << (8 * i);
    }
}

static void print_set(const char *name, uint8_t *set) {
    printf("%10s = ", name);
    for (int i = 0; i < 8; i++) {
        printf("%02x ", set[i]);
        if (i == 3)
            printf(" ");
    }
    printf("\n");
}

void sched_print(sched_t *s) {
    printf("t = %.2fus\n", s->t * 6.4 / 1000);
    print_set("pfc_day", s->pfc_day);
    print_set("pfc_night", s->pfc_night);
    print_set("circwl", s->circwl);
    print_set("packbl", s->packbl);

    printf("port_map = ");

    for (int i = 0; i < NPORT; i++) {
        if (i % 16 == 0 && i > 0) printf("           ");
        printf("%2d ", s->portmap[i]);
        if ((i + 1) % 16 == 0) printf("\n");
        else if ((i + 1) % 8 == 0) printf(" ");
    }
}

void sw_write(uint16_t addr, uint8_t * s, size_t n) {
    assert(n <= 48);
    
    packer.start(ROUTE_SW, uint8_t(n));

    // address
    packer.c(uint8_t(addr >> 8));
    packer.c(uint8_t(addr));

    // content
    packer.str(s, n);

    packer.send();
    usleep(10000);
}

void sw_write_byte(uint16_t addr, uint8_t c) {
    sw_write(addr, &c, 1);
}

void sw_init(bool dynamic) {
    sw_write_byte(0x000F, 0x10);
    sw_write_byte(0x0009, 0xf0);
    sw_write_byte(0x000B, 0x82); // 600mVppd
    // sw_write_byte(0x000B, 0x84); // 800mVppd
    sw_write_byte(0x000A, 0x00);
    
    sw_write_byte(0x0003, (dynamic ? 0x98 : 0x88));
}

void ctrl_select_debug(uint64_t master, uint64_t slave) {
    packer.start_cmd(CMD_DEBUG_SEL);
    packer.u64(master);
    packer.u64(slave);
    packer.send();
}

void ctrl_ask_for_counter() {
    packer.start_cmd(CMD_STAT);
    packer.pad(sizeof(struct stat_counter_t));
    packer.pad(8); // just for safety on the last set
    // packer.print();
    packer.send();
}

bool is_stat_counter(void *content, size_t size, stat_counter_t *c) {
    uint8_t *p = (uint8_t *)(content);
    if (size < 16 + sizeof(struct stat_counter_t)) { return false; }
    if (!(p[6] == ROUTE_CTRL && p[14] == CMD_STAT)) { return false; }

    // hprint(content, size);
    p += 16; // skip the first two sets
    
    if (c != NULL) {
        for (int i = 0; i < NHOST; i++) {
            c->stats[i].circ.nbyte = u32(&p[0]);
            c->stats[i].circ.npacket = u32(&p[4]);
            c->stats[i].circ.dropped = u32(&p[8]);

            c->stats[i].pack.nbyte = u32(&p[12]);
            c->stats[i].pack.npacket = u32(&p[16]);
            c->stats[i].pack.dropped = u32(&p[20]);
            p += 24;
        }

        c->week = u32(&p[0]);
        c->t_of_week = u32(&p[4]);
        c->t = u64(&p[8]);
        p += 16;

        c->week_slave = u32(&p[0]);
        c->t_of_week_slave = u32(&p[4]);
        c->t_slave = u64(&p[8]);
        p += 16;
    }
    
    return true;
}

const uint16_t LOOPBACK_SIZE = 60;

void ctrl_loopback_test(uint8_t id) {
    packer.start_cmd(CMD_LOOPBACK, id);
    for (uint16_t i = 0; i < LOOPBACK_SIZE; i++) {
        packer.c(uint8_t(i));
    }
    packer.send();
}

bool is_loopback(void *content, size_t size) {
    if (size < 16) { return false; }
    uint8_t *p = (uint8_t *)(content);
    if (p[6] == ROUTE_CTRL && p[14] == CMD_LOOPBACK) { return true; }
    return false;
}

bool check_loopback(void *content, size_t size, uint8_t *id) {
    if (size != 16 + LOOPBACK_SIZE) { return false; }
    uint8_t *p = (uint8_t *)(content);
    if (id != NULL) { *id = p[15]; }

    p += 16;

    for (uint16_t i = 0; i < LOOPBACK_SIZE; i++) {
        if (p[i] != uint8_t(i)) { return false; }
    }

    return true;
}
