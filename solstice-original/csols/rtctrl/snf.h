// myricom sniffer wrapper
struct MyriSnf {
    bool opened;
    snf_handle_t handle;
    snf_ring_t rx;
    snf_inject_t tx;

    MyriSnf() {
        init();
    }
    ~MyriSnf() {
        cleanup();
    }
    void init() {
        opened = false;
    }
    void cleanup() {
        close();
    }

    void panic_if(bool cond, const char *msg) {
        if (cond) {
            perror(msg);
            exit(-1);
        }
    }

    void panic(const char *msg) {
        panic_if(true, msg);
    }

    void open(uint32_t index) {
        assert(!opened);

        int ret;

        snf_init(SNF_VERSION_API); // okay to call multiple times

        ret = snf_open(index, 1, NULL, 0, -1, &handle);
        panic_if(ret != 0, "snf_open()");

        ret = snf_ring_open(handle, &rx);
        panic_if(ret != 0, "snf_ring_open()");

        ret = snf_inject_open(index, 0, &tx);
        panic_if(ret != 0, "snf_inject_open()");

        snf_start(handle);

        opened = true;
    }

    void close() {
        if (!opened) return;

        int ret;

        ret = snf_ring_close(rx);
        panic_if(ret != 0, "snf_ring_close()");

        ret = snf_close(handle);
        panic_if(ret != 0, "snf_close()");

        ret = snf_inject_close(tx);
        panic_if(ret != 0, "snf_inject_close()");

        opened = false;
    }

    void * recv(size_t *size, uint64_t *timestamp) {
        if (!opened) {
            return NULL;
        }

        struct snf_recv_req recv_req;
        int ret;

        ret = snf_ring_recv(rx, 0, &recv_req);

        if (ret == 0) {
            if (size) *size = recv_req.length;
            if (timestamp) *timestamp = recv_req.timestamp;

            return recv_req.pkt_addr;
        } else if (ret == EAGAIN) {
            return NULL;
        } else {
            panic("snf_ring_recv()");
            return NULL;
        }
    }

    bool send(void *data, size_t n) {
        if (!opened) {
            return false;
        }

        while (1) {
            int ret = snf_inject_send(tx, 0, data, n);
            if (ret == 0) return true;
            panic_if(ret != EAGAIN, "snf_inject_send()");
        }
    }
};

