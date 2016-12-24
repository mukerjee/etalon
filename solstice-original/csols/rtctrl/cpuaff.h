void RunOnCPU(unsigned int cpu) {
    cpu_set_t cpuset;

    CPU_ZERO(&cpuset);
    CPU_SET(cpu, &cpuset);
    if (sched_setaffinity(getpid(), sizeof(cpuset), &cpuset)) {
        perror("sched_setaffinity");
    }
}
