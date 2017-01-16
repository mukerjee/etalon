#include "sols.h"

/* imports */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* private functions */
static int _sols_mat_init(sols_mat_t *s, int nhost);
static void sols_mat_cleanup(sols_mat_t *s);

static int _sols_day_init(sols_day_t *s, int nhost);
static void sols_day_cleanup(sols_day_t *s);
static int _sols_init(sols_t *s, int nhost);

static void madd(sols_mat_t *res, sols_mat_t *a, sols_mat_t *b);
/* static void vsort(sols_mat_t *m); */ /* sort the index */
static void mcpy(sols_mat_t *dest, sols_mat_t *src);
static void mthres(sols_mat_t *res, sols_mat_t *m, uint64_t thres);
static void mappend(sols_mat_t *m, int index, uint64_t v);
/* static void mdiv(sols_mat_t *res, uint64_t d); */
static void svsort(sols_sumvec_t *sv, int nhost);


/* note: we start the init function names with an underscore
 * because it does not clear the structure or handle init failure */

/* macros */
#define NLANE(nhost) ((nhost) * (nhost))

#ifdef __APPLE__
#define FMT_U64 "llu"
#else
#define FMT_U64 "lu"
#endif

void
mprint(sols_mat_t * m) {
    int i, j;
    uint64_t v;

    for (i = 0; i < m->nhost; i++) {
        for (j = 0; j < m->nhost; j++) {
            if (j > 0) printf(" ");
            v = m->m[i*m->nhost + j];
            if (v == 0) {
                printf("   .");
            } else {
                printf("%4" FMT_U64, m->m[i*m->nhost + j]);
            }
        }
        printf("\n");
    }
    printf("\n");
}

static void
shuffle(int *a, int n) {
    int i;
    int stride;
    int swap;
    int t;

    for (i = 0; i < n-1; i++) {
        stride = rand() % (n - i);
        if (stride > 0) {
            swap = i + stride;
            t = a[i];
            a[i] = a[swap];
            a[swap] = t;
        }
    }
}

static void
randperm(int *a, int n) {
    int i;
    for (i = 0; i < n; i++) {
        a[i] = i;
    }

    shuffle(a, n);
    /*
    for (i = 0; i < n; i++) {
    	printf("%d ", a[i]);
    }
    printf("\n");
    */
}

/* should be already zeroed */
static int
_sols_mat_init(sols_mat_t *s, int nhost) {
    int nlane;

    s->nhost = nhost;

    nlane = NLANE(nhost);
    s->v = (int *)(malloc(sizeof(int) * nlane));
    s->m = (uint64_t *)(malloc(sizeof(uint64_t) * nlane));
    s->vi = (int *)(malloc(sizeof(int) * nlane));

    if (!s->v || !s->m) {
        return -1;
    }

    assert(s->n == 0);
    memset(s->m, 0, sizeof(uint64_t) * NLANE(nhost));

    return 0;
}

static void
sols_mat_cleanup(sols_mat_t *s) {
    free(s->v);
    free(s->m);
    free(s->vi);
}

void
sols_mat_clear(sols_mat_t *s) {
    int i;

    for (i = 0; i < s->n; i++) {
        s->m[s->v[i]] = 0;
    }
    s->n = 0;
}

static void
mappend(sols_mat_t *s, int index, uint64_t v) {
    assert(s->m[index] == 0);

    s->m[index] = v;
    s->v[s->n] = index;
    s->vi[index] = s->n;

    s->n++;
}

static int
sols_mat_check(sols_mat_t *s) {
    int i;
    int index;

    for (i = 0; i < s->n; i++) {
        index = s->v[i];
        if (s->m[index] == 0) {
            return 1;
        }
        if (s->vi[index] != i) {
            return 1;
        }
    }

    return 0;
}

void
sols_mat_set(sols_mat_t *s, int r, int c, uint64_t v) {
    uint64_t * pt;
    int index;
    int last;
    int i;
    uint64_t old;

    index = r * s->nhost + c;
    pt = &s->m[index];
    old = *pt;
    *pt = v;

    if (old == 0) {
        if (v == 0) {
            return;
        }

        s->v[s->n] = index;
        s->vi[index] = s->n;
        s->n++;
    } else { /* old > 0 */
        if (v > 0) {
            return;
        }

        /* remove the element from non-zero element list */
        assert(s->n > 0);

        i = s->vi[index]; /* the position in v */
        assert(i < s->n);

        if (i != s->n - 1) {
            /* move the last non-zero element to position i */
            last = s->v[s->n - 1];
            s->v[i] = last;
            s->vi[last] = i;
        }

        s->n--;
    }
}

void
sols_mat_add(sols_mat_t *s, int r, int c, uint64_t v) {
    uint64_t * pt;
    uint64_t old;
    int index;

    if (v == 0) {
        return;
    }

    index = r * s->nhost + c;
    pt = &s->m[index];
    old = *pt;
    assert(old + v != 0);
    *pt = old + v;

    if (old == 0) {
        s->v[s->n] = index;
        s->vi[index] = s->n;
        s->n++;
    }
}

uint64_t
sols_mat_get(sols_mat_t *s, int r, int c) {
    return s->m[r * s->nhost + c];
}

static int
_sols_day_init(sols_day_t *s, int nhost) {
    s->input_ports = (int *)(malloc(sizeof(int) * nhost));
    if (!s->input_ports) {
        return -1;
    }
    s->is_dummy = (int *)(malloc(sizeof(int) * nhost));
    if (!s->is_dummy) {
        return -1;
    }
    return 0;
}

static void
sols_day_cleanup(sols_day_t *s) {
    free(s->input_ports);
    free(s->is_dummy);
}

int
sols_init(sols_t *s, int nhost) {
    int ret;

    memset(s, 0, sizeof(sols_t));
    ret = _sols_init(s, nhost);
    if (ret) {
        sols_cleanup(s);
    }

    return ret;
}

static int
_sols_sumvec_init(sols_sumvec_t **v, int nhost) {
    *v = (sols_sumvec_t *)(malloc(sizeof(sols_sumvec_t) * nhost));
    if (!(*v)) {
        return -1;
    }
    return 0;
}

static void
svreset(sols_sumvec_t *v, int nhost) {
    int i;
    for (i = 0; i < nhost; i++) {
        v[i].i = i;
        v[i].s = 0;
    }
}

static int
svcmp(const void *a, const void *b) {
    const sols_sumvec_t *x = (const sols_sumvec_t *)(a);
    const sols_sumvec_t *y = (const sols_sumvec_t *)(b);
    uint64_t sa = x->s;
    uint64_t sb = y->s;

    if (sa > sb) {
        return -1;
    }
    if (sa < sb) {
        return 1;
    }
    if (x->i < y->i) {
        return -1;
    }
    if (x->i > y->i) {
        return 1;
    }
    return 0;
}

static void
svsort(sols_sumvec_t *v, int nhost) {
    qsort(v, nhost, sizeof(sols_sumvec_t), svcmp);
}

static int
_sols_index_init(sols_index *index, int nhost) {
    index->v = (int *)(malloc(sizeof(int) * nhost));
    index->vi = (int *)(malloc(sizeof(int) * nhost));

    if (!index->v || !index->vi) {
        return -1;
    }
    return 0;
}

static void
sols_index_cleanup(sols_index *index) {
    free(index->v);
    free(index->vi);
}

static int
_sols_init(sols_t *s, int nhost) {
    int ret;
    int i;

    s->nhost = nhost;

    ret = _sols_mat_init(&s->future, nhost);
    ret |= _sols_mat_init(&s->queued, nhost);
    ret |= _sols_mat_init(&s->bw_limit, nhost);
    for (i = 0; i < SOLS_MAX_NDAY; i++) {
        ret |= _sols_day_init(&s->sched[i], nhost);
    }

    ret |= _sols_mat_init(&s->request, nhost);
    ret |= _sols_mat_init(&s->demand, nhost);
    ret |= _sols_mat_init(&s->leftover, nhost);
    ret |= _sols_mat_init(&s->target, nhost);
    ret |= _sols_mat_init(&s->stuffed, nhost);
    ret |= _sols_mat_init(&s->left, nhost);
    ret |= _sols_mat_init(&s->stage, nhost);
    ret |= _sols_sumvec_init(&s->row_sum, nhost);
    ret |= _sols_sumvec_init(&s->col_sum, nhost);
    if (ret) {
        return -1;
    }

    s->index = (sols_index *)(malloc(sizeof(sols_index) * nhost));
    if (!s->index) {
        return -1;
    }
    memset(s->index, 0, sizeof(sols_index) * nhost);
    for (i = 0; i < nhost; i++) {
        ret |= _sols_index_init(&s->index[i], nhost);
    }
    if (ret) {
        return -1;
    }

    s->output_ports = (int *)(malloc(sizeof(int) * nhost));
    if (!s->output_ports) {
        return -1;
    }
    s->searched = (int *)(malloc(sizeof(int) * nhost));
    if (!s->searched) {
        return -1;
    }
    s->permbuf = (int *)(malloc(sizeof(int) * nhost * nhost));
    if (!s->permbuf) {
        return -1;
    }
    s->permbuf2 = (int *)(malloc(sizeof(int) * nhost * nhost));
    if (!s->permbuf2) {
        return -1;
    }

    /* default settings */
    s->night_len = 30;
    s->week_len = 1500;
    s->min_day_len = 160;
    s->avg_day_len = 300;
    s->day_len_align = 4;

    s->pack_bw = 125;
    s->link_bw = 1250;

    return 0;
}

void
sols_cleanup(sols_t *s) {
    int i;

    sols_mat_cleanup(&s->future);
    sols_mat_cleanup(&s->queued);
    sols_mat_cleanup(&s->bw_limit);

    for (i = 0; i < SOLS_MAX_NDAY; i++) {
        sols_day_cleanup(&s->sched[i]);
    }

    sols_mat_cleanup(&s->request);
    sols_mat_cleanup(&s->demand);
    sols_mat_cleanup(&s->leftover);
    sols_mat_cleanup(&s->target);
    sols_mat_cleanup(&s->stuffed);
    sols_mat_cleanup(&s->left);
    sols_mat_cleanup(&s->stage);
    free(s->row_sum);
    free(s->col_sum);

    if (s->index) {
        for (i = 0; i < s->nhost; i++) {
            sols_index_cleanup(&s->index[i]);
        }
    }
    free(s->index);

    free(s->output_ports);
    free(s->searched);
}

static void
sols_trim(sols_t *s) {
    int i;
    int index;
    uint64_t v;
    uint64_t th;
    sols_mat_t *d, *t;

    if (s->skip_trim) {
        return;
    }

    d = &s->demand;
    t = &s->target;
    th = s->link_bw * s->min_day_len;
    sols_mat_clear(t);

    for (i = 0; i < d->n; i++) {
        index = d->v[i];
        v = d->m[index];
        assert(v > 0);
        if (v > th) {
            mappend(t, index, v);
        }
    }
}

static void
sols_align(sols_t *s) {
    int i;
    int index;
    uint64_t v;
    uint64_t align;
    uint64_t r;
    sols_mat_t *t;

    align = s->link_bw;
    if (align <= 1) {
        return;
    }

    t = &s->target;
    for (i = 0; i < t->n; i++) {
        index = t->v[i];
        v = t->m[index];
        assert(v > 0);
        if (v <= align) {
            v = align;
        } else {
            r = v % align;
            if (r >= align / 2) {
                v += align - r;
            } else {
                v -= r;
            }
        }

        t->m[index] = v / align;
    }
}

/*
 * The sharp solstice stuffer.
 *
 * A stuffer stuffs a general matrix into a doubly-stochastic matrix.  There
 * are possibly mutiple ways to stuff a matrix. They take different amount
 * of time to calculate, and will lead to different decomposing results as
 * well. Therefore, stuff needs some extra care.
 *
 * Sharp stuffer works as follows:
 * 1. it calculates the max sum of each row and col, which is the stuff
 *    target.
 * 2. it try to stuff the non-zero elements first.
 * 3. we calculate the space left for each row and col.
 * 4. we the repeatedly pick the the row and col that has the largest space,
 *    and stuff the element that this row and col intersects
 *
 * By doing this, the result matrix is likely to be skewed and hence will
 * lead to better decomposing results on the slicing step.
 */
static void
sols_stuff(sols_t *s) {
    int i, j;
    uint64_t norm;
    uint64_t v;
    int r, c;
    int index;
    int nhost;
    sols_mat_t *t;
    sols_mat_t *res;
    uint64_t row_delta;
    uint64_t col_delta;
    uint64_t delta;
    sols_sumvec_t *rsum, *csum;

    /* prepare */

    t = &s->target;
    nhost = s->nhost;
    norm = 0;
    rsum = s->row_sum;
    csum = s->col_sum;

    svreset(rsum, nhost);
    svreset(csum, nhost);

    /* calculate the sums */

    for (i = 0; i < t->n; i++) {
        index = t->v[i];
        v = t->m[index];
        assert(v > 0);
        r = index / nhost;
        c = index % nhost;
        rsum[r].s += v;
        csum[c].s += v;
    }

    for (i = 0; i < nhost; i++) {
        if (rsum[i].s > norm) {
            norm = rsum[i].s;
        }
    }
    for (i = 0; i < nhost; i++) {
        if (csum[i].s > norm) {
            norm = csum[i].s;
        }
    }

    res = &s->stuffed;
    mcpy(res, t);

    if (norm == 0) {
        return;
    }
    /* printf("norm=%lu\n", norm); */

    /* first round stuffing: non-zero elements */

    shuffle(res->v, res->n);
    for (i = 0; i < res->n; i++) {
        index = res->v[i];
        r = index / nhost;
        c = index % nhost;

        assert(norm >= rsum[r].s);
        assert(norm >= csum[c].s);
        row_delta = norm - rsum[r].s;
        col_delta = norm - csum[c].s;
        delta = (row_delta < col_delta) ? row_delta : col_delta;
        /* printf("rd=%lu cd=%lu d=%lu\n", row_delta, col_delta, delta); */
        if (delta > 0) {
            res->m[index] += delta;
            rsum[r].s += delta;
            csum[c].s += delta;
        }
    }

    /* second round stuffing: all elements */

    svsort(rsum, nhost);
    svsort(csum, nhost);

    i = 0;
    j = 0;

    while (1) {
        while (rsum[i].s >= norm) {
            assert(rsum[i].s == norm);
            i++;
            if (i == nhost) {
                return;
            }
        }

        while (csum[j].s >= norm) {
            assert(csum[j].s == norm);
            j++;
            assert(j < nhost);
        }

        row_delta = norm - rsum[i].s;
        col_delta = norm - csum[j].s;
        delta = (row_delta < col_delta) ? row_delta : col_delta;

        assert(delta > 0);
        /* printf("delta=%lu\n", delta); */

        r = rsum[i].i;
        c = csum[j].i;

        v = sols_mat_get(res, r, c);
        assert(v == 0);
        mappend(res, r * nhost + c, delta);

        rsum[i].s += delta;
        csum[j].s += delta;
    }
}

static int
sols_slice_search(sols_t *s, int *ins, int dest) {
    sols_index *index;
    int i, ind, n;
    int *outs;
    int *searched;
    int src, d;
    int * permbuf;

    index = &s->index[dest];
    outs = s->output_ports; /* src->dest */
    searched = s->searched;
    n = index->n;

    if (n == 0) {
        return 0;
    }

    permbuf = (int *)(malloc(sizeof(int) * n));
    randperm(permbuf, n);

    for (i = 0; i < n; i++) {
        ind = permbuf[i];
        src = index->v[ind];

        if (searched[src]) {
            continue;
        }

        searched[src] = 1;

        d = outs[src];
        if (d < 0 || sols_slice_search(s, ins, d)) {
            outs[src] = dest;
            ins[dest] = src;

            free(permbuf);
            return 1;
        }
    }

    free(permbuf);
    return 0;
}

static sols_day_t *
sols_slice(sols_t *s) {
    sols_day_t *ret;
    sols_index *index; /* index of non-zero elements */
    /* the index is the only thing we need to look
     * at here for matching*/
    int i, j;
    int nhost;
    int *ins, *outs;
    int src, dest;
    int nmatch;

    ret = &s->sched[s->nday];
    nhost = s->nhost;
    ins = ret->input_ports;
    outs = s->output_ports;

    /* init the port map */
    for (i = 0; i < nhost; i++) {
        ins[i] = -1; /* input port for each dest */
    }
    for (i = 0; i < nhost; i++) {
        outs[i] = -1; /* output port for each src */
    }
    nmatch = 0;

    /* pre-matching, find a maximal */
    randperm(s->permbuf, nhost);
    for (i = 0; i < nhost; i++) {
        dest = s->permbuf[i];

        index = &s->index[dest];
        assert(ins[dest] < 0);

        randperm(s->permbuf2, index->n);
        for (j = 0; j < index->n; j++) {
            src = index->v[s->permbuf2[j]];
            if (outs[src] < 0) {
                outs[src] = dest;
                ins[dest] = src;
                nmatch++;
                break;
            }
        }
    }

    for (dest = 0; dest < nhost; dest++) {
        if (ins[dest] >= 0) {
            continue;
        }

        /* clearing the searched marks */
        memset(s->searched, 0, sizeof(int) * nhost);

        if (sols_slice_search(s, ins, dest)) {
            nmatch++;
        } else {
            goto not_found;
        }
    }

    s->nday++;
    return ret;

not_found:
    return NULL;
}

static void
sols_decompose(sols_t *s) {
    uint64_t thres;
    uint64_t base;
    uint64_t week_len;
    /* uint64_t budget; */
    sols_day_t *day;
    int nhost;
    int src, dest;
    uint64_t v;
    uint64_t day_len;
    int i;
    int index;
    int r, c;
    int n;
    int last;
    uint64_t minBase;

    nhost = s->nhost;

    assert(s->min_day_len > s->night_len);
    base = s->min_day_len;
    /* make sure the duty cycle is larger than 50% */
    minBase = s->night_len * 2;
    if (base < minBase) {
        base = minBase;
    }
    if (base == 0) {
        base = 1;
    }

    week_len = s->week_len;
    assert(week_len > 0);
    /* budget = s->week_len; */

    thres = base;
    while (thres < week_len / 4) {
        thres *= 2;
    }

    mcpy(&s->left, &s->stuffed);
    /* mprint(&s->stuffed); */

    s->nday = 0;
    for ( /*empty*/; thres >= base; thres /= 2) {
        if (s->left.n == 0) {
            break;
        }
        assert(s->left.n >= nhost);
        // printf("%lu\n", thres);

        mthres(&s->stage, &s->left, thres);

        /*
        printf("thres=%lu\n", thres);
        mprint(&s->left);
        printf("stage (after thres):\n");
        mprint(&s->stage);
        */

        if (s->stage.n < nhost) {
            continue;
        }

        /* build the index */
        for (i = 0; i < nhost; i++) {
            s->index[i].n = 0;
        }
        for (i = 0; i < s->stage.n; i++) {
            index = s->stage.v[i];
            v = s->stage.m[index];
            r = index / nhost;
            c = index % nhost;
            n = s->index[c].n;
            s->index[c].v[n] = r;
            s->index[c].vi[r] = n;
            s->index[c].n++;
        }

        while (1) {
            /* mprint(&s->left);
            printf("\n"); */
            day = sols_slice(s);
            if (!day) {
                break;
            }

            /* search for the day len */
            day_len = 0;
            for (dest = 0; dest < nhost; dest++) {
                src = day->input_ports[dest];
                v = sols_mat_get(&s->left, src, dest);
                assert(v > 0);
                if (day_len == 0 || v < day_len) {
                    day_len = v;
                }

                v = sols_mat_get(&s->target, src, dest);
                if (v > 0) {
                    day->is_dummy[dest] = 0;
                } else {
                    day->is_dummy[dest] = 1;
                }
            }

            day->len = day_len + s->night_len;
            /* printf("got day: %lu\n", day->len); */
            assert(day->len >= s->min_day_len);
            /*
            if (day->len > budget) {
                day->len = budget;
            }
            */

            for (dest = 0; dest < nhost; dest++) {
                src = day->input_ports[dest];
                /* printf("%d -> %d\n", src, dest); */
                assert(src >= 0); /* always perfect matching */

                v = sols_mat_get(&s->stage, src, dest);
                assert(sols_mat_get(&s->left, src, dest) == v);

                assert(v >= day_len);
                sols_mat_set(&s->left, src, dest, v-day_len);

                if (v - day_len >= thres) {
                    sols_mat_set(&s->stage, src, dest, v-day_len);
                } else {
                    sols_mat_set(&s->stage, src, dest, 0);

                    index = s->index[dest].vi[src];
                    n = s->index[dest].n;
                    assert(index < n);
                    if (index == n - 1) {
                        s->index[dest].n--;
                    } else {
                        last = s->index[dest].v[n-1];
                        s->index[dest].v[index] = last;
                        s->index[dest].vi[last] = index;
                        s->index[dest].n--;
                    }
                }
            }

            /* budget -= day->len; */
            /* printf("day_len: %lu, budget: %lu\n", day->len, budget); */
            /* if (budget < s->min_day_len) {
                goto out;
            } */
        }
    }

    /* out: */
    return;
}

void
sols_scale(sols_t *s) {
    sols_day_t *day;
    int i;
    uint64_t week_len;
    uint64_t gap;
    uint64_t r;
    uint64_t t;
    uint64_t min_day_len;
    uint64_t min_day_len2;
    uint64_t align;
    uint64_t target;
    int max_nday;
    uint64_t min;
    int smallest;
    sols_day_t swap;

    int nhost;

    nhost = s->nhost;

    /* add an idle day */
    if (s->nday == 0) {
        s->nday = 1;
        day = &s->sched[0];
        day->len = s->week_len;
        for (i = 0; i < nhost; i++) {
            day->input_ports[i] = (i + 1) % nhost;
            day->is_dummy[i] = 1;
        }

        assert(s->week_len % s->day_len_align == 0);
        return;
    }

    week_len = 0;
    align = s->day_len_align;
    for (i = 0; i < s->nday; i++) {
        s->sched[i].len /= align;
        if (s->sched[i].len % align != 0) {
            s->sched[i].len++;
        }
        week_len += s->sched[i].len;
    }

    assert(s->week_len % align == 0);
    target = s->week_len / align;

    if (week_len == target) {
        /* do nothing */
    } else if (week_len < target) {
        gap = target - week_len;

        r = gap / s->nday;
        if (r > 0) {
            for (i = 0; i < s->nday; i++) {
                s->sched[i].len += r;
            }
        }
        r = gap % s->nday;
        for (t = 0; t < r; t++) {
            s->sched[t].len++;
        }
    } else {
        assert(week_len > target);

        min_day_len = s->min_day_len / align;
        if (s->min_day_len % align != 0) {
            min_day_len++;
        }
        min_day_len2 = s->night_len * 2 / align;
        if (s->night_len * 2 % align != 0) {
            min_day_len2++;
        }
        if (min_day_len < min_day_len2) {
            min_day_len = min_day_len2;
        }
        if (min_day_len > target) {
            min_day_len = target;
        }

        max_nday = (int)(target / min_day_len);
        while (max_nday < s->nday) {
            assert(s->nday > 1);
            smallest = 0;
            min = s->sched[0].len;
            for (i = 1; i < s->nday; i++) {
                if (s->sched[i].len <= min) {
                    smallest = i;
                    min = s->sched[i].len;
                }
            }

            week_len -= s->sched[smallest].len;
            s->nday--;
            if (smallest < s->nday) {
                swap = s->sched[smallest];
                s->sched[smallest] = s->sched[s->nday];
                s->sched[s->nday] = swap;
            }
        }

        while (week_len > target) {
            for (i = 0; i < s->nday; i++) {
                if (week_len > target && s->sched[i].len > min_day_len) {
                    s->sched[i].len--;
                    week_len--;
                    if (week_len == target) {
                        goto out;
                    }
                }
            }
        }
out:
        assert(week_len == target);
        /* double-check */
        week_len = 0;
        for (i = 0; i < s->nday; i++) {
            week_len += s->sched[i].len;
        }
        assert(week_len == target);
    }

    for (i = 0; i < s->nday; i++) {
        s->sched[i].len *= align;
    }
}

static void
sols_norm_down(sols_t *s) {
    int i, j;
    int index;
    int r, c;
    uint64_t v;
    sols_mat_t * left;
    sols_mat_t * dem;
    sols_mat_t * request;
    int nhost;
    sols_sumvec_t *rsum;
    sols_sumvec_t *csum;
    uint64_t link_bw;
    uint64_t inc_delta;
    int delta_valid;
    uint64_t t;
    uint64_t nopen;
    int need_recount;
    /*
    uint64_t max;
    uint64_t max2;
    */

    if (s->skip_norm_down) {
        mcpy(&s->demand, &s->request);
        sols_mat_clear(&s->left);
        return;
    }

    link_bw = s->link_bw * s->week_len;
    nhost = s->nhost;
    rsum = s->row_sum; /* row budget */
    csum = s->col_sum; /* col budget */

    memset(rsum, 0, sizeof(sols_sumvec_t) * nhost);
    memset(csum, 0, sizeof(sols_sumvec_t) * nhost);

    request = &s->request;
    left = &s->leftover;
    dem = &s->demand;

    mcpy(left, request); /* everything is left over at first */
    sols_mat_clear(dem);

    /* calculate the available budget */
    for (i = 0; i < nhost; i++) {
        rsum[i].s = link_bw;
    }
    for (i = 0; i < nhost; i++) {
        csum[i].s = link_bw;
    }

    while (1) {
        /*
        printf("request:\n");
        mprint(request);
        printf("left:\n");
        mprint(left);
        */
        while (1) {
            need_recount = 0;
            inc_delta = link_bw + 1;
            delta_valid = 0;

            for (i = 0; i < nhost; i++) {
                rsum[i].i = 0;
            }
            for (i = 0; i < nhost; i++) {
                csum[i].i = 0;
            }
            for (i = 0; i < left->n; i++) {
                index = left->v[i];
                v = left->m[index];
                r = index / nhost;
                c = index % nhost;
                assert(v > 0);
                rsum[r].i++; /* non-zero cnt */
                csum[c].i++; /* non-zero cnt */
                if (v > link_bw) {
                    v = link_bw;
                }
                if (inc_delta > v) {
                    inc_delta = v;
                    delta_valid = 1;
                }
            }

            for (i = 0; i < nhost; i++) {
                if (rsum[i].s == 0) {
                    continue;
                }
                nopen = (uint64_t)(rsum[i].i);
                if (nopen == 0) {
                    t = 0;
                } else {
                    t = rsum[i].s / nopen;
                }
                if (t == 0) {
                    rsum[i].s = 0;
                    for (j = 0; j < nhost; j++) {
                        sols_mat_set(left, i, j, 0);
                    }
                    need_recount = 1;
                    break;
                } else if (inc_delta > t) {
                    inc_delta = t;
                    delta_valid = 1;
                }
            }

            if (need_recount) {
                continue;
            }

            for (i = 0; i < nhost; i++) {
                if (csum[i].s == 0) {
                    continue;
                }
                nopen = (uint64_t)(csum[i].i);
                if (nopen == 0) {
                    t = 0;
                } else {
                    t = csum[i].s / nopen;
                }
                if (t == 0) {
                    csum[i].s = 0;
                    for (j = 0; j < nhost; j++) {
                        sols_mat_set(left, j, i, 0);
                    }
                    need_recount = 1;
                    break;
                } else if (inc_delta > t) {
                    inc_delta = t;
                    delta_valid = 1;
                }
            }

            if (need_recount) {
                continue;
            } else {
                break;
            }
        }

        if (!delta_valid) {
            break;    /* it is over here */
        }

        /* mprint(left); */
        for (i = 0; i < left->n; i++) {
            index = left->v[i];
            v = left->m[index];
            r = index / nhost;
            c = index % nhost;
            /* printf("%d, %d = %"FMT_U64"\n", r, c, v); */

            assert(v > 0);
            assert(v >= inc_delta);
            assert(rsum[r].s >= inc_delta);
            assert(csum[c].s >= inc_delta);

            assert(rsum[r].i > 0);
            assert(csum[c].i > 0);

            rsum[r].i--;
            csum[c].i--;
            rsum[r].s -= inc_delta;
            csum[c].s -= inc_delta;
            sols_mat_set(left, r, c, v - inc_delta);
            if (v == inc_delta) {
                i--; /* the element is removed */
            }
            sols_mat_add(&s->demand, r, c, inc_delta);

            if (rsum[r].s == 0) { /* a row just closed */
                rsum[r].i = -1;
            }
            if (csum[c].s == 0) {
                csum[c].i = -1;
            }
            /* printf("%d %d\n", i, left->n); */
        }

        for (i = 0; i < nhost; i++) {
            if (rsum[i].i < 0) {
                /* printf("row %d closing\n", i); */
                assert(rsum[i].i == -1);
                for (j = 0; j < nhost; j++) {
                    sols_mat_set(left, i, j, 0);
                }
                rsum[i].i = 0;
            }
        }

        for (i = 0; i < nhost; i++) {
            if (csum[i].i < 0) {
                /* printf("col %d closing\n", i); */
                assert(csum[i].i == -1);
                for (j = 0; j < nhost; j++) {
                    sols_mat_set(left, j, i, 0);
                }
                csum[i].i = 0;
            }
        }
        /* mprint(left); */

        for (i = 0; i < nhost; i++) {
            /* printf("%d, rsum=%d csum=%d\n", i, rsum[i].i, csum[i].i); */
            assert(rsum[i].i == 0);
            assert(csum[i].i == 0);
        }
    }

    /* recalculate the leftover */
    sols_mat_clear(left);

    for (i = 0; i < request->n; i++) {
        index = request->v[i];
        v = request->m[index];
        r = index / nhost;
        c = index % nhost;
        if (v > 0) {
            t = sols_mat_get(dem, r, c);
            assert(t <= v);
            if (t < v) {
                mappend(left, index, v - t);
            }
        }
    }

    /* TODO: should we stuff the little small gaps once more? */
}

static void
sols_throttle(sols_t *s) {
    int i;
    sols_mat_t *m;
    int index;
    uint64_t v;
    uint64_t bw;
    uint64_t nbyte;
    uint64_t bw_min, bw_max;
    int dest, src;
    int nhost;
    sols_day_t *d;

    nhost = s->nhost;

    sols_mat_clear(&s->bw_limit);
    for (i = 0; i < s->nday; i++) {
        d = &s->sched[i];
        for (dest = 0; dest < nhost; dest++) {
            if (d->is_dummy[dest] != 0) {
                continue;
            }
            src = d->input_ports[dest];
            /* assert(src != dest); */
            assert(d->len > s->night_len);
            sols_mat_add(&s->bw_limit, src, dest, d->len - s->night_len);
        }
    }

    assert(s->link_bw >= s->pack_bw);
    bw_min = s->link_bw - s->pack_bw;
    bw_max = s->link_bw;

    m = &s->bw_limit;
    for (i = 0; i < m->n; i++) {
        index = m->v[i];
        v = m->m[index];
        assert(v > 0);
        nbyte = s->demand.m[index];
        assert(nbyte > 0);

        bw = nbyte / v;
        if (nbyte % v != 0 || bw == 0) {
            bw++;
        }
        if (bw > bw_max) {
            bw = bw_max;
        }
        if (bw < bw_min) {
            bw = bw_min;
        }
        /*
        printf("lane=%d dem=%" FMT_U64
        		" tick=%" FMT_U64 " bw=%" FMT_U64 "\n",
        	index, nbyte, v, bw);
        */
        m->m[index] = bw;
    }
}

void
sols_schedule(sols_t *s) {
    madd(&s->request, &s->future, &s->queued);
    sols_norm_down(s); /* norm down the demand into a satisfiable one */
    sols_trim(s); /* trim the small elements */
    sols_align(s); /* align the target */
    sols_stuff(s); /* stuff the target into a doubly-stochastic matrix */
    sols_decompose(s); /* decompose the matrix into slices */
    sols_scale(s); /* scaling the day length to fit total week length */
    /* TODO: merge, scale, interleave and shuffle */
    sols_throttle(s); /* calculate the bandwidth throttling */
}

void
sols_test(sols_t *s) {
    sols_norm_down(s);
}

void
sols_roundrobin(sols_t *s) {
    uint64_t each;
    uint64_t nbig;
    int i, j;
    int nhost;
    int nday;
    sols_day_t *day;
    uint64_t bw;
    uint64_t week_len;

    nhost = s->nhost;
    assert(nhost >= 2);
    nday = nhost - 1;

    assert(s->week_len % s->day_len_align == 0);

    week_len = s->week_len / s->day_len_align;
    each = week_len / (uint64_t)(nday);
    assert(each * s->day_len_align >= s->min_day_len);
    assert(each * s->day_len_align >= s->night_len);

    nbig = week_len % (uint64_t)(nday);
    s->nday = nday;

    for (i = 0; i < nday; i++) {
        day = &s->sched[i];
        day->len = each;
        if ((uint64_t)(i) < nbig) {
            day->len++;
        }

        for (j = 0; j < nhost; j++) {
            day->input_ports[j] = (j + 1 + i) % nhost;
            day->is_dummy[j] = 0;
        }

        day->len *= s->day_len_align;
    }

    sols_mat_clear(&s->bw_limit);
    assert(s->link_bw > s->pack_bw);
    bw = s->link_bw - s->pack_bw;
    for (i = 0; i < nhost; i++) {
        for (j = 0; j < nhost; j++) {
            mappend(&s->bw_limit, i*nhost+j, bw);
        }
    }
}

static void
madd(sols_mat_t *res, sols_mat_t *a, sols_mat_t *b) {
    int i;
    int index;
    uint64_t v;

    sols_mat_clear(res);
    for (i = 0; i < a->n; i++) {
        index = a->v[i];
        v = b->m[index];
        mappend(res, index, a->m[index] + v);
    }

    for (i = 0; i < b->n; i++) {
        index = b->v[i];
        if (a->m[index] > 0) {
            continue;
        }
        mappend(res, index, b->m[index]);
    }
}

/*
static void
mdiv(sols_mat_t *m, uint64_t d) {
    int i;
    int index;

    if (d == 0) {
        return;
    }

    for (i = 0; i < m->n; i++) {
        index = m->v[i];
        m->m[index] /= d;
    }
}
*/

static void
mcpy(sols_mat_t *dest, sols_mat_t *src) {
    int i;
    int index;
    uint64_t v;

    sols_mat_clear(dest);

    for (i = 0; i < src->n; i++) {
        index = src->v[i];
        v = src->m[index];
        assert(v > 0);

        mappend(dest, index, v);
    }
}

static void
mthres(sols_mat_t *res, sols_mat_t *m, uint64_t th) {
    int i;
    int index;
    uint64_t v;

    sols_mat_clear(res);

    for (i = 0; i < m->n; i++) {
        index = m->v[i];
        v = m->m[index];
        assert(v > 0);
        if (v >= th) {
            mappend(res, index, v);
        }
    }
}

int
sols_check(sols_t *s) {
    uint64_t week_len;
    int i;
    sols_mat_t *m;
    int nhost;
    int index;
    int r, c;
    uint64_t v;
    uint64_t sum;
    int pass;
    int ret;

    ret = 0;
    nhost = s->nhost;

    /* check week length */
    week_len = 0;
    for (i = 0; i < s->nday; i++) {
        week_len += s->sched[i].len;
    }
    if (week_len != s->week_len) {
        printf("week_len %" FMT_U64 "!=%" FMT_U64 "\n",
               week_len, s->week_len);
        ret++;
    }

    /* check if the stuffed matrix is doubly stochastic */
    m = &s->stuffed;
    for (i = 0; i < nhost; i++) {
        s->row_sum[i].s = 0;
    }
    for (i = 0; i < nhost; i++) {
        s->col_sum[i].s = 0;
    }

    for (i = 0; i < m->n; i++) {
        index = m->v[i];
        r = index / nhost;
        c = index / nhost;
        v = m->m[index];
        s->row_sum[r].s += v;
        s->col_sum[c].s += v;
    }

    sum = s->row_sum[0].s;
    pass = 1;
    for (i = 0; i < nhost; i++) {
        if (s->row_sum[i].s != sum) {
            pass = 0;
        }
    }
    for (i = 0; i < nhost; i++) {
        if (s->col_sum[i].s != sum) {
            pass = 0;
        }
    }
    if (!pass) {
        printf("stuffed matrix not doubly-stochastic\n");
        ret++;
    }

    if (sols_mat_check(&s->left)) {
        printf("leftover is not a valid matrix\n");
        ret++;
    }

    return ret;
}

/*
static int
vcmp(const void *a, const void *b) {
    int x = *((int *)(a));
    int y = *((int *)(b));
    if (x < y) {
        return -1;
    }
    if (x > y) {
        return 1;
    }
    return 0;
}

static void
vsort(sols_mat_t *m) {
    qsort(m->v, m->n, sizeof(int), vcmp);
}
*/

int sols_day_is_dummy(sols_day_t * day, int dest) {
    return day->is_dummy[dest];
}

int sols_day_input_port(sols_day_t * day, int dest) {
    return day->input_ports[dest];
}

uint64_t sols_bw_limit(sols_t * s, int lane) {
    return s->bw_limit.m[lane];
}
