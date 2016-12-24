struct NicSync {
    uint32_t magic;
    uint32_t weekCount;
    uint8_t rateValid;
    uint8_t remapValid;
    uint8_t doRemap;
    uint8_t applyNow;
    uint8_t rates[NQUEUE];
    uint8_t remaps[NQUEUE];

    bool valid() {
        return rateValid != 0 || remapValid != 0;
    }

    void clear() {
        memset(this, 0, sizeof(*this));
    }

    void init() {
        magic = htonl(0x372453AE);
        doRemap = true;
        remapValid = true;
        rateValid = true;
        applyNow = false;
    }

    void setRemaps(uint8_t *remaps) {
        remapValid = true;
        doRemap = true;
        memcpy(this->remaps, remaps, NQUEUE);
    }

    void mapRate(uint8_t q, uint8_t rate) {
        setRemap(q, q, rate);
    }

    void setRemap(uint8_t q, uint8_t remap, uint8_t rate) {
        if (q == 0) {
            return;
        }
        assert(q != 0);

        remaps[q] = remap;
        rates[q] = rate;
    }



    void setNoremap() {
        memset(this->remaps, 0, NQUEUE);
        remapValid = true;
        doRemap = false;
    }

    void setCircRate(int q, uint8_t rate) {
        if (rates[q] != 0) {
            rates[q] = rate;
        }
    }

    void setPackRate(uint8_t r) {
        rates[0] = r;
    }

    void setRates(uint8_t *rates) {
        rateValid = true;
        memcpy(this->rates, rates, NQUEUE);
    }
};
