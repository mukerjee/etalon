#ifndef _mcp_printf_h
#define _mcp_printf_h

#define MCP_PRINT_MAGIC 0x7072696e /* prin */
#define MCP_PRINT_BUF_SIZE 8192U
struct mcp_print {
  struct mcp_print_info {
    unsigned last_rtc;
    unsigned rtc_off;
    unsigned magic;
    unsigned written;
    unsigned read; /* for host use only */
    unsigned buf_off;
  } info;
};

#if defined __lanai__
#define printf mcp_printf
#define sprintf mcp_sprintf
int mcp_raw_printf(const char *,...) __attribute__((regparm(0)));
int printf(const char *,...) __attribute__((regparm(0)));
int sprintf(char *,const char*, ...) __attribute__((regparm(0)));
int mcp_puts (const char *s);
#endif

#define MCP_PRINT_OFF_ZE(highmem_start)  (ZE_DUMP_OFF(highmem_start) - sizeof(struct mcp_print_info))

#define MCP_PRINT_OFF_LX(pci_offset)  ((unsigned)(pci_offset) - sizeof(struct mcp_print_info))

#endif /* _mcp_printf_h */
