struct WinDemand {
    uint64_t d[NLANE];

    void print(FILE *fout) {
        mprint(fout, d);
    }

    void clear() {
        memset(d, 0, sizeof(uint64_t) * NLANE);
    }
};

const uint16_t CtrlRtPort = 8203;
const int WinMax = 5000;

static volatile bool pleaseDump = false;
void dumpNotify(int) {
    pleaseDump = true;
}

struct Main {
    Sol8 s8;
    CtrlGen cgen;
    Flags flags;

    WinDemand *dem;

    /* for logging */
    WinDemand *rec;
    Days *sch;

    int nwin; // number of windows received from the control oracle
    char bw[NHOST * NHOST];

    const char * demandSave;

    Main() {
        // srand(time(0));
        srand(3721);
        dem = new WinDemand[WinMax]; // a window of demand
        rec = new WinDemand[WinMax];
        sch = new Days[WinMax];
        nwin = 0;

        demandSave = NULL;
    }

    ~Main() {
        delete [] dem;
        delete [] rec;
        delete [] sch;
    }

    void serve() {
        init();

        if (flags.useOracleDemand) {
            int fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
            assert(fd != -1);

            int opt = 1;
            int ret = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR,
                                 &opt, sizeof(int));
            assert(ret == 0);

            sockaddr_in addr;
            addr.sin_family = AF_INET;
            addr.sin_addr.s_addr = INADDR_ANY;
            addr.sin_port = htons(CtrlRtPort);

            ret = bind(fd, (struct sockaddr*)&addr, sizeof(addr));
            assert(ret != -1);

            ret = listen(fd, 3);
            assert(ret == 0);
            fprintf(stderr, "open-loop scheduling with oracle demand\n");

            while (1) {
                socklen_t len = sizeof(addr);
                int client = accept(fd, (struct sockaddr *)&addr, &len);
                assert(client != -1);

                fprintf(stderr, "client connected\n");

                uint8_t * buf = (uint8_t *)(dem);
                size_t i = 0;
                size_t bufSize = size_t(WinMax) * sizeof(WinDemand);

                while (i < bufSize) {
                    ssize_t ret = recv(client, &buf[i], bufSize - i, 0);
                    if (ret == -1 || ret == 0) {
                        break;
                    }
                    i += ret;
                }

                if (ret == -1) {
                    perror("recv(client)");
                }

                close(client);
                fprintf(stderr, "client disconnected\n");

                if (ret == 0) {
                    nwin = i / sizeof(WinDemand);
                    fprintf(stderr, "(%d weeks to run)\n", nwin);

                    runDemand();
                }
            }
        } else if (flags.useEstimatedDemand) {
            runDemand();
        } else {
            runDemand(); // run with default demand
        }
    }

    void saveDump(int cur) {
        if (demandSave != NULL) {
            FILE *fout = fopen(demandSave, "w");
            assert(fout);

            int cnt = 0;

            for (int i = 0; i < WinMax; i++) {
                int pt = (cur + i) % WinMax;
                Days * s = &sch[pt];
                if (!s->valid) {
                    continue;
                }

                fprintf(fout, "w%d:", cnt++);
                uint64_t * d = rec[pt].d;
                for (int src = 0; src < NHOST; src++) {
                    for (int dest = 0; dest < NHOST; dest++) {
                        uint64_t v = d[src * NHOST + dest];
                        if (v == 0) {
                            fprintf(fout, " .");
                        } else {
                            fprintf(fout, " "F_U64, v);
                        }
                    }
                    fprintf(fout, ";");
                }
                fprintf(fout, "\n");

                for (int j = 0; j < s->nday; j++) {
                    fprintf(fout, "d%d:", j);
                    s->d[j].print(fout);
                }

                fprintf(fout, "bw:");
                for (int src = 0; src < NHOST; src++) {
                    for (int dest = 0; dest < NHOST; dest++) {
                        char b = s->bw[src * NHOST + dest];
                        fprintf(fout, " %d", int(b));
                    }
                    fprintf(fout, ";");
                }
                fprintf(fout, "\n");

                fprintf(fout, "\n");
            }
            fclose(fout);
        }
    }

    void _makeA2A(uint64_t * d) {
        for (int src = 0; src < NHOST; src++) {
            for (int dest = 0; dest < NHOST; dest++) {
                if (src == dest) {
                    d[src *NHOST + dest] = 0;
                } else {
                    d[src * NHOST + dest] = 13 * 1250 * 1500 / 100;
                }
            }
        }
    }

    void makeA2A() {
        // a testing all 2 all demand
        nwin = 4000; // 6 second

        for (int i = 0; i < nwin; i++) {
            dem[i].clear();
            _makeA2A(dem[i].d);
        }
    }

    void makeRandp() {
        nwin = 5;

        for (int i = 0; i < nwin; i++) {
            dem[i].clear();

            int dests[NHOST];
            for (int j = 0; j < NHOST; j++) {
                dests[j] = j;
            }
            for (int j = 0; j < NHOST; j++) {
                int swap = rand() % (NHOST - j);
                if (swap == 0) {
                    continue;
                }
                int t = dests[j];
                dests[j] = dests[j+swap];
                dests[j+swap] = t;
            }

            for (int src = 0; src < NHOST; src++) {
                int dest = dests[src];
                if (src == dest) continue;
                dem[i].d[src * NHOST + dest] = 90 * 1250 * 1500 / 100;
            }
        }
    }

    void printDays(vector<Day> & days) {
        int i = 1;

        for (vector<Day>::iterator it = days.begin();
                it != days.end(); it++) {
            printf("d%d:  ", i);
            it->print(stdout);
            i++;
        }
    }

    int runTest() {
        makeA2A();
        // makeRandp();
        init();
        runDemand();

        return 0;
    }

    void init() {
        if (!flags.testRun) {
            cgen.snfOpen();
        }
    }

    int runDemand() {
        int cur = 0;
        int count = 0;

        if (!flags.testRun) {
            sleep(1);
            cgen.clearBuf();
            cgen.resetWeekId();
        }

        memset(bw, 0, sizeof(char) * NHOST * NHOST);

        bool isStart = true;

        uint64_t defaultDem[NLANE] = {
            0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0,

            0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0,
        };
        _makeA2A(defaultDem);

        while (1) {
            if (flags.nweek != 0 && count >= flags.nweek) {
                break;
            }
            if (flags.useOracleDemand && count >= nwin) {
                break;
            }

            if (!flags.testRun) {
                cgen.waitSig();
            }

            uint64_t *d;

            if (flags.useOracleDemand) {
                d = dem[count].d;
            } else if (flags.useEstimatedDemand) {
                d = cgen.demRecv.dem;
            } else {
                d = defaultDem;
            }

            // take a log
            if (!flags.noLogging) {
                memcpy(rec[cur].d, d, sizeof(uint64_t) * NLANE);
            }

            vector<Day> & days = flags.alwaysRoundr ?
                                 s8.rrobin(bw) :
                                 s8.sched(d, bw);

            if (!flags.testRun) {
                cgen.sendSched(days, bw, count, isStart);
            } else {
                printf("## W%d\n", cur+1);
                printDays(days);
                mcprint(stdout, bw);
                printf("\n");
            }

            if (!flags.noLogging) {
                Days & curSch = sch[cur];

                curSch.valid = true;
                curSch.nday = int(days.size());
                for (int i = 0; i < curSch.nday; i++) {
                    memcpy(&curSch.d[i], &(days[i]), sizeof(Day));
                }
                memcpy(&curSch.bw, bw, sizeof(char) * NHOST * NHOST);
            }

            cur++;
            cur = cur % WinMax;

            if (pleaseDump) {
                printf("saving dump...\n");
                saveDump(cur);
                pleaseDump = false;
                printf("saved.\n");
            }
            isStart = false;

            count++;
        }

        if (!flags.testRun) {
            vector<Day> & days = s8.rrobin(bw);
            if (!flags.testRun) {
                cgen.sendSched(days, bw, cur);
                printf("## end with round-robin\n");
            }

            printf("#dem recv: %d\n", cgen.ndemRecv);
        }

        return 0;
    }
};
