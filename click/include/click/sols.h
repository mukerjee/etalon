#if defined (__cplusplus)
extern "C" {
#endif

#include <stdint.h>

#define SOLS_MAX_NDAY 256

/* sols_mat_t is defines a sparse matrix structure */
typedef struct _sols_mat_t {
    int nhost; /* the demension of the matrix */
    int n; /* number of non-zero elements */
    int *v; /* index of non-zero elements */
    uint64_t *m; /* all elements */
    int *vi; /* the index of non-zero elements in v */
} sols_mat_t;

void sols_mat_clear(sols_mat_t *s);
void sols_mat_set(sols_mat_t *s, int r, int c, uint64_t v);
uint64_t sols_mat_get(sols_mat_t *s, int r, int c);

typedef struct _sols_day_t {
    uint64_t len;
    int *input_ports; /* input port for each output port */
    int *is_dummy; /* if the input port mapping is just a dummy one */
    /* the index is the output port, and the value is the input port */
    /* -1 means no assignment to the output port */
} sols_day_t;

typedef struct _sols_sumvec_t {
    int i;
    uint64_t s;
} sols_sumvec_t;

typedef struct _sols_index {
    int n;  /* number of non-zero elements */
    int *v; /* index of non-zero elements */
    int *vi; /* the index of non-zero elements in v */
} sols_index;

/* all the stuff the for algorithm */
typedef struct _sols_t {
    /* configurations */
    int nhost;

    uint64_t night_len;
    uint64_t week_len;
    uint64_t min_day_len;
    uint64_t avg_day_len;
    uint64_t day_len_align;

    uint64_t pack_bw;
    uint64_t link_bw;

    uint8_t skip_trim;
    uint8_t skip_norm_down;

    /* input: the demand */
    sols_mat_t future;
    sols_mat_t queued;

    /* output: the schedule */
    int nday;
    sols_day_t sched[SOLS_MAX_NDAY];
    sols_mat_t bw_limit;

    /* temporary buffers */
    sols_mat_t request; /* demand request to schedule */
    sols_mat_t demand;  /* demand that will schedule actually */
    sols_mat_t leftover; /* demand that is left over for the future */
    sols_mat_t target;  /* aligned decompose target */
    sols_mat_t stuffed; /* stuffed decompose target */
    sols_mat_t left;    /* demant left for scheduling */
    sols_mat_t stage;   /* staging buffer */

    sols_sumvec_t *row_sum;
    sols_sumvec_t *col_sum;

    sols_index *index;  /* index of non-zero elements for each column */
    int *output_ports;
    int *searched;
    int *permbuf;
    int *permbuf2;
} sols_t;

int sols_init(sols_t *s, int nhost); /* returns 0 on success */
void sols_cleanup(sols_t *s);

void sols_schedule(sols_t *s);
void sols_roundrobin(sols_t *s);
int sols_check(sols_t *s); /* check and returns the number of errors */

void sols_test(sols_t *s); /* just for testing */

int sols_day_is_dummy(sols_day_t * day, int dest);
int sols_day_input_port(sols_day_t * day, int dest);
uint64_t sols_bw_limit(sols_t *s, int lane);

#if defined (__cplusplus)
}
#endif
