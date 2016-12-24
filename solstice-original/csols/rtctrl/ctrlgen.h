MyriSnf theSnf;

void snfSend(void *buf, size_t n) {
    theSnf.send(buf, n);
}

const size_t SRC_ROUTE_OFFSET = 6;

// a fast and dirty control packet generator
struct CtrlGen {
    Packer *packer;
    UdpSender udpSender;
    size_t headerSize;
    NicSync nicSyncs[NHOST];
    DemRecv demRecv; // demand receiver

    uint32_t lastid;
    int ndemRecv;

    bool twoPackQueue;

    CtrlGen() {
        packer = udpSender.prepare(sizeof(NicSync));
        headerSize = packer->size();
        assert(headerSize == 42);
        for (uint8_t i = 0; i < NHOST; i++) {
            nicSyncs[i].clear();
        }

        lastid = 0;
        ndemRecv = 0;

        twoPackQueue = false;
    }

    void snfOpen(uint32_t i=1) {
        theSnf.open(i);
        ctrl_set_sender(snfSend);
    }

    void resetWeekId() {
        lastid = 0;
    }

    void sendSched(vector<Day> & days, char *bw,
                   int weekId=0, bool isStart=false) {
        uint64_t total = 0;
        unsigned int nday = days.size();
        assert(nday <= 128);

        unsigned int nset = nday / 16;
        if (nday % 16 != 0) {
            nset++;
        }

        for (int i = 0; i < NHOST; i++) {
            nicSyncs[i].clear();
            nicSyncs[i].setPackRate(10);
            nicSyncs[i].init();
        }

        uint8_t pfc_default[NHOST];
        uint8_t pfc_default_night;
        for (int src = 0; src < NHOST; src++) {
            pfc_default[src] = 0x1; // enable packet queue
            if (twoPackQueue && src < 4) {
                pfc_default[src] |= ((uint8_t)(0x7) << 4);
            }
        }
        pfc_default_night = 0x1;
        if (twoPackQueue) {
            pfc_default_night |= 0x70;
        }

        if (twoPackQueue) {
            for (unsigned int i = 0; i < nday; i++) {
                Day & d = days[i];
                for (int dest = 0; dest < 4; dest++) {
                    int src = d.srcOf(dest);
                    bool dum = d.dummyDest(dest);
                    if (src >= 4) {
                        continue;
                    }
                    if (dum) {
                        continue;
                    }
                    int qid = (dest + 4 - src) % 4;
                    if (qid == 0) {
                        continue;
                    }
                    // clear the bit on the circuit-packet queue
                    pfc_default[src] &= ((uint8_t)(0xff ^ (0x1 << (qid + 3))));
                }
            }
        }

        for (unsigned int i = 0; i < nset; i++) {
            unsigned int start = i * 16;
            assert(nday > start);
            unsigned int n = nday - start;
            if (n > 16) {
                n = 16;
            }
            sched_t scheds[16];

            for (unsigned int j = start; j < n; j++) {
                Day & d = days[j];
                sched_t & sched = scheds[j];

                for (int src = 0; src < NHOST; src++) {
                    sched.pfc_day[src] = pfc_default[src];
                    sched.pfc_night[src] = pfc_default_night;
                    sched.circwl[src] = 0;
                    sched.packbl[src] = 0;
                }

                memset(sched.portmap, 0, sizeof(uint8_t) * NPORT);

                assert(d.len % 4 == 0); // must be aligned
                sched.t = d.len * 625 / 4; // convert us to cycles (6.4ns)
                total += sched.t;
                for (int dest = 0; dest < NHOST; dest++) {
                    int src = d.srcOf(dest);
                    bool dum = d.dummyDest(dest);
                    if (!dum) {
                        int qid = (dest + NHOST - src) % NHOST;
                        if (!twoPackQueue) {
                            sched.pfc_day[src] |= (uint8_t(0x1)) << qid;
                        } else {
                            if (dest < 4 && src < 4) {
                                int qid2 = (dest + 4 - src) % 4;
                                sched.pfc_day[src] |= (uint8_t(0x1)) << qid2;
                            }
                        }

                        sched.circwl[src] |= (uint8_t(0x1)) << dest;
                        // anything non-zero is okay here
                        // will be reapply with the correct rate later
                        nicSyncs[src].mapRate(qid, 90);
                    }
                    sched.portmap[dest] = uint8_t(src);
                }
            }

            ctrl_set_sched(scheds, uint8_t(n), uint8_t(start));
        }

        // apply the rates
        for (int src = 0; src < NHOST; src++) {
            for (int dest = 0; dest < NHOST; dest++) {
                if (src == dest) {
                    continue;
                }
                int lane = src * NHOST + dest;
                int qid = (dest + NHOST - src) % NHOST;
                assert(qid != 0);
                nicSyncs[src].setCircRate(qid, (uint8_t)(bw[lane]));
            }
        }

        assert(total >= 1000 * 625 / 4);
        ctrl_set_weeksig_pos(total - 400 * 625 / 4);
        ctrl_commit_sched(uint8_t(nday), isStart);
        syncNics(weekId);
    }

    // send the queue mappings to end-hosts
    void syncNics(int weekId=0) {
        for (uint8_t i = 0; i < NHOST; i++) {
            packer->set(SRC_ROUTE_OFFSET, 0xf0 + i);
            packer->trunc(headerSize);

            nicSyncs[i].weekCount = weekId;
            packer->pack(&nicSyncs[i], sizeof(NicSync));

            // packer->print();
            packer->snfSend(&theSnf);
        }
    }

    bool isSig(void *p, size_t n, uint32_t *weekid) {
        uint8_t *pt = (uint8_t *)p;

        if (n < 60) return false;
        bool ret = (pt[14] == 0x1E && pt[15] == 0xEC);
        if (ret && weekid) {
            uint32_t i = uint32_t(pt[36]);
            i |= uint32_t(pt[37]) << 8;
            i |= uint32_t(pt[38]) << 16;
            i |= uint32_t(pt[39]) << 24;

            *weekid = i;
        }

        return ret;
    }

    void clearBuf() {
        size_t n;
        uint64_t t;
        for (int i = 0; i < 100000; i++) {
            theSnf.recv(&n, &t);
        }
    }

    uint32_t waitSig() {
        size_t n;
        uint64_t t;

        while (1) {
            void *ret = theSnf.recv(&n, &t);
            if (!ret) continue;
            uint32_t weekid;
            /*if (n != 60 && n != 632) {
                fprintf(stderr, "recved: nbyte=%lu\n", n);
                hprint(ret, n);
            } */

            if (demRecv.tryParse((uint8_t *)(ret), n) == 0) {
                ndemRecv++;
                /*
                fprintf(stderr, "demand recv id=%d src=%d\n",
                        demRecv.id(),
                        demRecv.last_src
                       );
                */

                if (ndemRecv % 3 == 0) {
                    printf("#dem recv: %d\n", ndemRecv);
                }
            } else if (isSig(ret, n, &weekid)) {
                if (weekid != lastid + 1 && lastid != 0) {
                    fprintf(stderr, "week id jump %d->%d\n",
                            lastid, weekid);
                }
                lastid = weekid;
                return weekid;
            } else {
                // just skip
            }
        }
    }

    void recvDemOnly() {
        size_t n;
        uint64_t t;
        while (1) {
            void *ret = theSnf.recv(&n, &t);
            if (!ret) continue;

            if (demRecv.tryParse((uint8_t *)(ret), n) == 0) {
                ndemRecv++;
                /*
                fprintf(stderr, "demand recv id=%d src=%d\n",
                        demRecv.id(),
                        demRecv.last_src
                       );
                */
                break;
            }
        }
    }
};

