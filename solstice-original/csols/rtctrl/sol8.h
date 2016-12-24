struct Day {
    uint64_t len;
    uint64_t in2out;
    uint64_t out2in;

    void clear() {
        in2out = out2in = 0;
    }

    void set(int src, int dest, bool dummy=false) {
        assert(src >= 0 && src < NHOST);
        assert(dest >= 0 && dest < NHOST);

        uint64_t bsrc = ((uint64_t)(src & 0x7));
        uint64_t bdest = ((uint64_t)(dest & 0x7));
        if (dummy) {
            bsrc |= 0x8;
            bdest |= 0x8;
        }

        out2in |= bsrc << (4 * dest);
        in2out |= bdest << (4 * src);
    }

    static int m(uint64_t c, int off, uint64_t mask) {
        return (int)((c >> (4 * off)) & mask);
    }

    int srcOf(int dest) {
        return m(out2in, dest, 0x7);
    }
    int srcCodeOf(int dest) {
        return m(out2in, dest, 0xf);
    }
    bool dummyDest(int dest) {
        return m(out2in, dest, 0x8) != 0;
    }

    int destOf(int src) {
        return m(in2out, src, 0x7);
    }
    int destCodeOf(int src) {
        return m(in2out, src, 0xf);
    }
    bool dummySrc(int src) {
        return m(in2out, src, 0x8) != 0;
    }

    void print(FILE *fout) {
        for (int i = 0; i < NHOST; i++) {
            if (dummySrc(i)) {
                fprintf(fout, "%d .%d ", i, destOf(i));
            } else {
                fprintf(fout, "%d->%d ", i, destOf(i));
            }
        }

        fprintf(fout, " // t=" F_U64 "  ", len);
        fprintf(fout, "\n");
    }
};

struct Days {
    bool valid;
    Day d[NHOST * NHOST];
    int nday;
    char bw[NHOST * NHOST];

    Days() {
        memset(this, 0, sizeof(Days));
    }
};

struct Sol8 {
    sols_t sols;
    vector<Day> days; // port maps
    bool checking;
    // int bwlim[NLANE];

    Sol8() {
        assert(NHOST == 8);
        sols_init(&sols, NHOST);
        checking = true;
    }

    ~Sol8() {
        sols_cleanup(&sols);
        sols_mat_clear(&sols.queued); // we will never touch this
    }

    void setDays() {
        days.clear();

        for (int i = 0; i < sols.nday; i++) {
            sols_day_t & d = sols.sched[i];
            Day day = { 0, 0, 0 };
            day.len = d.len;
            for (int dest = 0; dest < NHOST; dest++) {
                int src = d.input_ports[dest];
                bool dum = (d.is_dummy[dest] == 0) ? false : true;
                assert(src >= 0 && src < NHOST);
                day.set(src, dest, dum);
                /*
                printf("src=%d, dest=%d\n", src, dest);
                printf("dest=%d, src=%d\n", day.destOf(src), day.srcOf(dest));
                printf("%d\n", day.dummyDest(src));
                day.print(stdout);
                */
            }

            days.push_back(day);
            // day.print(stdout);
        }
    }

    // returns the port map
    vector<Day> & sched(uint64_t *dem, char *bw) {
        // just a hack for bandwidth limit for now
        // TODO: need a better way to deal with bw limit
        /*
        for (int i = 0; i < NLANE; i++) {
            bwlim[i] = 90;
        }
        */

        sols_mat_clear(&sols.queued);
        sols_mat_clear(&sols.future);
        for (int i = 0; i < NLANE; i++) {
            uint64_t v = dem[i];
            if (v == 0) continue;
            int r = i / NHOST;
            int c = i % NHOST;
            if (r == c) continue; // never set the diagnal
            // printf("%d %d = %lu\n", r, c, v);
            sols_mat_set(&sols.future, r, c, v);
        }

        // run it!
        sols_schedule(&sols);

        // sanity checking;
        if (checking) {
            sols_check(&sols);
        }

        /*{
            sols_t & s = sols;
            printf("%d days\n", s.nday);
            for (int i = 0; i < s.nday; i++) {
                sols_day_t *day;
                int src, dest;

                day = &s.sched[i];
                printf("day #%d: T=%lu\n", i, day->len);
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
        }*/

        setDays();
        copyBwLimit(bw);
        return days;
    }

    void copyBwLimit(char *bw) {
        if (bw) {
            for (int i = 0; i < NLANE; i++) {
                bw[i] = sols_bw_limit(&sols, i) * 100 / sols.link_bw;
            }
        }
    }

    vector<Day> & rrobin(char *bw) {
        sols_roundrobin(&sols);
        setDays();
        copyBwLimit(bw);
        return days;
    }
};
