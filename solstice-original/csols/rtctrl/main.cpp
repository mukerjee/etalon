#include "inc.h"

int main(int argc, char **argv) {
    printf("pid=%d\n", getpid());
    printf("`$ kill -%d %d` to dump\n", SIGUSR1, getpid());

    sighandler_t ret = signal(SIGUSR1, dumpNotify);
    assert(ret != SIG_ERR);

    RunOnCPU(5);
    Main m;

    m.flags.parse(argc, argv);
    // m.testRun = false;
    // m.alwaysRoundr = false;
    // m.skipLogging = false;

    // m.recvDemEst = true;
    m.demandSave = "t.dmp";

    m.serve();
    // m.runTest();

    return 0;
}
