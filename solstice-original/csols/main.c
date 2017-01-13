#include "sols.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <time.h>

#ifdef __APPLE__
#define FMT_U64 "%llu"
#else
#define FMT_U64 "%lu"
#endif

static void
mprint(sols_mat_t * m) {
    int i, j;
    uint64_t v;

    for (i = 0; i < m->nhost; i++) {
        for (j = 0; j < m->nhost; j++) {
            if (j > 0) printf(" ");
            v = m->m[i*m->nhost + j];
            if (v == 0) {
                printf(".");
            } else {
                printf(FMT_U64, v);
            }
        }
        printf("\n");

    }
}

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

/*
static void
mmul(sols_mat_t * m, uint64_t x) {
    int nlane;
    int i;

    assert(x > 0);
    nlane = m->nhost * m->nhost;
    for (i = 0; i < nlane; i++) {
        m->m[i] *= x;
    }
}
*/

#define NHOST 4

int
mainTest() {
    int i;

    sols_t s;
    uint64_t dat[] = {
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,

        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
    };

    sols_init(&s, NHOST);
    /*
    s.week_len = 1;
    s.day_len_align = 1;
    s.night_len = 1;
    s.min_day_len = 1;
    s.link_bw = 10;
    */

    mset(&s.future, dat);

    sols_schedule(&s);

    sols_test(&s);
    mprint(&s.future);
    printf("%d days\n", s.nday);
    for (i = 0; i < s.nday; i++) {
        sols_day_t *day;
        int src, dest;

        day = &s.sched[i];
        printf("day #%d: T=" FMT_U64 "\n", i, day->len);
        for (dest = 0; dest < NHOST; dest++) {
            src = day->input_ports[dest];
            assert(src >= 0);
            if (day->is_dummy[dest]) {
                printf("  (%d -> %d)\n", src, dest);
            } else {
                printf("  %d -> %d\n", src, dest);
            }
        }
    }

    sols_cleanup(&s);

    return 0;
}

struct Timer {
    struct timespec tstart;
    struct timespec tend;
};

static void timer_start(struct Timer *tm) {
    int ret;

    ret = clock_gettime(CLOCK_MONOTONIC, &tm->tstart);
    assert(ret == 0);
}

uint64_t tsns(struct timespec * ts) {
    return (uint64_t)(ts->tv_sec) * (uint64_t)(1000000000) +
           (uint64_t)(ts->tv_nsec);
}

uint64_t timer_read(struct Timer *tm) {
    uint64_t from, to;
    int ret;

    ret = clock_gettime(CLOCK_MONOTONIC, &tm->tend);
    assert(ret == 0);

    from = tsns(&tm->tstart);
    to = tsns(&tm->tend);
    assert(to >= from);

    return to - from;
}

int
main() {
    //return mainTest();

    sols_t s;
    int i, j;
    struct Timer timer;
    uint64_t tread;
    int tries;

    uint64_t dat[NHOST * NHOST];

    sols_init(&s, NHOST); /* init for 8 hosts */
    s.night_len = 20;
    s.week_len = 3000;
    s.avg_day_len = 40;
    s.min_day_len = 40;
    s.day_len_align = 1;
    // s.skip_norm_down = 1;

    srand(time(0));

    for (tries = 0; tries < 30; tries++) {
        uint64_t cap = (s.week_len * s.link_bw / NHOST);

        for (i = 0; i < NHOST; i++) {
            for (j = 0; j < NHOST; j++) {
                if (i == j) {
                    continue;
                }

                dat[i * NHOST + j] = rand() % cap;
            }
        }

        /* setup the demand here */
        mset(&s.future, dat);
        /* mmul(&s.future, 1250); */

        timer_start(&timer);
        sols_schedule(&s);
        tread = timer_read(&timer);

        sols_check(&s);

        /* debugging */

        
        printf("[demand]\n");
        mprint(&s.demand);

        for (i = 0; i < s.nday; i++) {
            sols_day_t *day;
            int src, dest;

            day = &s.sched[i];
            printf("day #%d: T=" FMT_U64 "\n", i, day->len);
            for (dest = 0; dest < NHOST; dest++) {
                src = day->input_ports[dest];
                assert(src >= 0);
                if (day->is_dummy[dest]) {
                    printf("  (%d -> %d)\n", src, dest);
                } else {
                    printf("  %d -> %d\n", src, dest);
                }
            }
        }

        printf("(%.3fus)\n", (double)(tread) / 1e3);
    }

    sols_cleanup(&s);

    return 0;
}
