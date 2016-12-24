# Driver sources 
COMMON_SRCS += mx_common.c
COMMON_SRCS += mx_instance.c
COMMON_SRCS += mx_register.c
COMMON_SRCS += mx_lanai_command.c
COMMON_SRCS += kraw.c
COMMON_SRCS += mcp_wrapper_common.c
COMMON_SRCS += mx_lx.c
COMMON_SRCS += mx_lz.c
COMMON_SRCS += myri_snf_common.c
COMMON_SRCS += mx_klib.c
#M_SNF_KAGENT#COMMON_LIB_SRCS += snf_rx.c
COMMON_SRCS += myri_ptp_common.c

# ethernet
ETHER_COMMON_SRCS += mx_ether_common.c

# kernel lib top-level common
KERNEL_LIB_TOP_COMMON_SRCS += mal_kutils.c

# kernel lib support
KERNEL_LIB_SRCS += snf_api.c
KERNEL_LIB_SRCS += snf_api_params.c
KERNEL_LIB_SRCS += snf_tx.c

