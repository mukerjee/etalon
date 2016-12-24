void mprint(FILE * fout, uint64_t * d) {
    for (int i = 0; i < NHOST; i++) {
        for (int j = 0; j < NHOST; j++) {
            uint64_t v = d[i * NHOST + j];
            if (v == 0) {
                fprintf(fout, ". ");
            } else {
                fprintf(fout, F_U64 " ", v);
            }
        }
        fprintf(fout, "\n");
    }
}

void mcprint(FILE * fout, char * d) {
    for (int i = 0; i < NHOST; i++) {
        for (int j = 0; j < NHOST; j++) {
            char v = d[i * NHOST + j];
            if (v == 0) {
                fprintf(fout, ".   ");
            } else {
                fprintf(fout, "%3d ", v);
            }
        }
        fprintf(fout, "\n");
    }
}
