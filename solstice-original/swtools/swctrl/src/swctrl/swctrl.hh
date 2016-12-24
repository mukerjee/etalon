#pragma once

#include <cstddef>
#include <stdint.h>

const uint8_t NHOST = 8;
const size_t MAC_ADDR_SIZE = 6;

void sw_init(bool dynamic=true);
void sw_write_byte(uint16_t addr, uint8_t t);
void sw_write(uint16_t addr, uint8_t *s, size_t n);

bool is_weeksig(void *content, size_t size, uint32_t *nweek);

struct stat_number_t {
    uint32_t nbyte;
    uint32_t npacket;
    uint32_t dropped;
};

struct stat_counter_t {
    uint64_t t;
    uint32_t week;
    uint32_t t_of_week;

    uint64_t t_slave;
    uint32_t week_slave;
    uint32_t t_of_week_slave;

    struct {
        struct stat_number_t circ, pack;
    } stats[NHOST];
};
bool is_stat_counter(void *content, size_t size, stat_counter_t *c);
bool is_loopback(void *content, size_t size);
bool check_loopback(void *content, size_t size, uint8_t *id);

void ctrl_ask_for_counter();
void ctrl_loopback_test(uint8_t id);

void ctrl_set_sender(void (*sender)(void *, size_t));

void ctrl_reset();
void ctrl_reset_sched();

void ctrl_set_addr(uint8_t addrs[NHOST][MAC_ADDR_SIZE]);

struct ctrl_flags_t {
    bool send_pfc;
    bool send_weekend;
    bool send_weeksig;
    bool sync_internal;
    bool dpdk_frame_type;
};
void ctrl_set_flags(ctrl_flags_t *flags, ctrl_flags_t *slave_flags=NULL);

struct sender_conf_t {
    bool enable;
    uint16_t len;
    uint32_t pad;
};
void ctrl_config_senders(sender_conf_t *b3, sender_conf_t *f3);
void ctrl_set_weeksig_pos(uint64_t pos);

struct ctrl_timings_t {
    uint64_t t_night;
    uint64_t t_plan;
    uint64_t t_map;
    uint64_t t_sync;
    uint64_t t_switch;
};
void ctrl_set_timings(ctrl_timings_t *timings);

const uint8_t NPORT = 48;
struct sched_t {
    uint64_t t;
    uint8_t pfc_day[NHOST];
    uint8_t pfc_night[NHOST];
    uint8_t circwl[NHOST];
    uint8_t packbl[NHOST];
    uint8_t portmap[NPORT];
};

void sched_print(sched_t *s);

void ctrl_set_sched(sched_t *s, uint8_t n, uint8_t offset=0);
void ctrl_commit_sched(uint8_t nday, bool load_start=false);

void ctrl_select_debug(uint64_t master, uint64_t slave);
void ctrl_use_lanes(uint8_t master, uint8_t slave, 
        uint8_t *tx, uint8_t *rx);
void ctrl_config_lanes();
uint8_t out_port(uint8_t lane);
uint8_t in_port(uint8_t lane);

enum {
    LANE_D0 = 0,
    LANE_D1,
    LANE_D2,
    LANE_D3,
    LANE_E0,
    LANE_E1,
    LANE_E2,
    LANE_E3,
    LANE_F0,
    LANE_F1,
    LANE_F2,
    LANE_F3,
};

enum {
    POS_SE = 0,
    POS_SW = 1,
    POS_NE = 2,
    POS_NW = 3,
    POS_M1 = 4,
    POS_M2 = 5,
    POS_V1 = 6,
    POS_V2 = 7,
};

