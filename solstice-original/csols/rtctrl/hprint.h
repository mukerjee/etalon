void hprint(void *buf, size_t n) {
    uint8_t *p = (uint8_t *)(buf);

    for (size_t i = 0; i < n; i++) {
        printf("%02x", p[i]);
        if ((i + 1) % 16 == 0)
            printf("\n");
        else if ((i + 1) % 8 == 0)
            printf("  ");
        else if ((i + 1) % 4 == 0)
            printf(" ");
    }

    if (n % 16 != 0) {
        printf("\n");
    }
}
