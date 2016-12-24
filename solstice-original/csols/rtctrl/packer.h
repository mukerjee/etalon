const size_t PACKER_BUF_SIZE = 1500; // MTU size

struct Packer {
    uint8_t buf[PACKER_BUF_SIZE];
    size_t n;

    void clear() {
        n = 0;
    }
    size_t size() {
        return n;
    }
    size_t pack(void *content, size_t nbytes) {
        assert(n + nbytes <= PACKER_BUF_SIZE);

        memcpy(&buf[n], content, nbytes);
        n += nbytes;
        return n;
    }

    void trunc(size_t nbytes) {
        assert(nbytes <= PACKER_BUF_SIZE);
        n = nbytes;
    }

    bool snfSend(MyriSnf *snf) {
        // print();
        return snf->send(buf, n);
    }

    void set(size_t pos, uint8_t c) {
        buf[pos] = c;
    }

    void print() {
        for (size_t i = 0; i < n; i++) {
            printf("%02x ", buf[i]);
            if ((i+1) % 8 == 0) {
                printf("\n");
            }
        }
        printf("\n");
    }
};
