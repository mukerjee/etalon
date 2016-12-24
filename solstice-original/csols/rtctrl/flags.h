struct Flags {
    Flags() {
        memset(this, 0, sizeof(Flags));
    }

    bool alwaysRoundr;
    bool testRun;
    bool noLogging;
    int nweek;
    bool useEstimatedDemand;
    bool useOracleDemand;

    void printHelp(FILE *f) {
        static const char *msg =
            "usage: ctrlhost with solstice running in realtime\n"
            "  -r       always serve roundrobin\n"
            "  -t       just test run, don't send the schedule to the fpga\n"
            "  -q       skip logging\n"
            "  -n <i>   run for n weeks only\n"
            "  -d       use the estimated demand\n"
            "  -O       use the received oracle demand\n"
            "  -h       print help\n";
        fprintf(f, "%s", msg);
    }

    void parse(int argc, char **argv) {
        int opt;
        static const char *opts = "rtqn:dOh";
        while ((opt = getopt(argc, argv, opts)) != -1) {
            switch (opt) {
            case 'r':
                alwaysRoundr = true;
                break;
            case 't':
                testRun = true;
                break;
            case 'q':
                noLogging = true;
                break;
            case 'n':
                nweek = atoi(optarg);
                break;
            case 'd':
                useEstimatedDemand = true;
                break;
            case 'O':
                useOracleDemand = true;
                break;
            case 'h':
                printHelp(stdout);
                exit(0);
                break;
            default:
                printHelp(stderr);
                exit(-1);
                break;
            }
        }
    }
};
