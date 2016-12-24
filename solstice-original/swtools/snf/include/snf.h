/*************************************************************************
 * The contents of this file are subject to the MYRICOM SNIFFER10G
 * LICENSE (the "License"); User may not use this file except in
 * compliance with the License.  The full text of the License can found
 * in LICENSE.TXT
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied.  See
 * the License for the specific language governing rights and
 * limitations under the License.
 *
 * Copyright 2008-2009 by Myricom, Inc.  All rights reserved.
 ***********************************************************************/


#ifndef _snf_h
#define _snf_h

#ifndef MAL_KERNEL
#include <stdint.h>
#endif

#ifdef __cplusplus
extern "C"
{
#endif



/**
 * SNF API version number (16 bits)
 * Least significant byte increases for minor backwards compatible changes
 * in the API. Most significant byte increases for incompatible changes in the API
 *
 * 0x0003: Add injection support and 3 send counters in statistics.
 *
 * 0x0002: Add nic_bytes_recv counter to stats to help users calculate the
 *         amount of bandwidth that is actually going through the NIC
 *         port.
 */
#define SNF_VERSION_API 0x0003


typedef struct snf_handle *snf_handle_t;

/**
 * Initializes the sniffer library.
 *
 * @brief Initialize Sniffer Library with api_version == @ref SNF_VERSION_API
 *
 * @param api_version Must always be @ref SNF_VERSION_API
 *
 * @remarks This must be called before any other call to the sniffer
 *          library.
 * @remarks The library may be safely initialized multiple times, although
 *          the api_version shoudl be the same SNF_VERSION_API each time.
 */
int  snf_init(uint16_t api_version); /* must be SNF_VERSION_API */


/** Structure to map Interfaces to Sniffer board numbers */
struct snf_ifaddrs {
  struct snf_ifaddrs *snf_ifa_next;      /**< next item or NULL if last */
  const char *snf_ifa_name;       /**< interface name, as in ifconfig */
  uint32_t    snf_ifa_boardnum;   /**< snf board number */
  int         snf_ifa_maxrings;   /**< Maximum RX rings supported */
  uint8_t     snf_ifa_macaddr[6]; /**< MAC address */
  uint8_t     pad[2];             /**< Internal padding (ignore) */
  int         snf_ifa_maxinject;  /**< Maximum TX injection handles supported */
};

/** 
 * Get a list of Sniffer-capable ethernet devices.
 *
 * @param ifaddrs_o Library-allocated list of Sniffer-capable devices
 *
 * @remarks Much like getifaddrs, the user can traverse the list until
 *          snf_ifa_next is NULL.  The interface will show up if the
 *          ethernet driver sees the device but the interface does not
 *          have to be brought up with an IP address (i.e. no need to
 *          'ifconfig up').
 *
 * @post User should call @ref snf_freeifaddrs to free the memory that was
 *       allocated by the library.
 */
int  snf_getifaddrs(struct snf_ifaddrs **ifaddrs_o);

/**
 * Free the list of library allocated memory for @ref snf_getifaddrs
 *
 * @param ifaddrs Pointer to ifaddrs allocated via @ref snf_getifaddrs
 */
void snf_freeifaddrs(struct snf_ifaddrs *ifaddrs);

struct snf_recv_req;



enum snf_rss_params_mode { 
  SNF_RSS_FLAGS = 0, /**< Apply RSS using specified flags */
  SNF_RSS_FUNCTION = 1 /**< Apply RSS using user-defined function: Kernel API only */
};


enum snf_rss_mode_flags {
  SNF_RSS_IP        =  0x01, /**< Include IP (v4 or v6) SRC/DST addr in hash */
  SNF_RSS_SRC_PORT  =  0x10, /**< Include TCP/UDP SRC port in hash */
  SNF_RSS_DST_PORT  =  0x20  /**< Include TCP/UDP DST port in hash */
};
#define SNF_RSS_IPV4 SNF_RSS_IP /**< Alias for @ref SNF_RSS_IP since IPv4 and 
                                   IPv6 are always both enabled */


struct snf_rss_mode_function {
  
  int   (*rss_hash_fn)(struct snf_recv_req *r, void *context, 
                       uint32_t *hashval);
  
  void   *rss_context;
};


struct snf_rss_params {
  enum snf_rss_params_mode mode; /**< RSS mode */
  union {
    enum snf_rss_mode_flags      rss_flags; /**< RSS parameters for @ref SNF_RSS_FLAGS */ 
    struct snf_rss_mode_function rss_function; /**< RSS params for @ref SNF_RSS_FUNCTION */
  } params; /**< RSS parameter settings, according to the mode that is selected */
};



/**
 * Device can be process-sharable.  This allows multiple independent
 * processes to share rings on the capturing device.  This option can be
 * used to design a custom capture solution but is also used in libpcap
 * when multiple rings are requested.  In this scenario, each libpcap
 * device sees a fraction of the traffic if multiple rings are used unless
 * the @ref SNF_F_RX_DUPLICATE option is used in which case each libpcap
 * device sees the same incoming packets.
 */
#define SNF_F_PSHARED 0x1

/**
 * Device can duplicate packets to multiple rings as opposed to applying
 * RSS in order to split incoming packets across rings.  Users should be
 * aware that with N rings opened, N times the link bandwidth is necessary
 * to process incoming packets without drops.  The duplication happens in
 * the host rather than the NIC, so while only up to 10Gbits of traffic
 * crosses the PCIe, N times that bandwidth is necessary on the host.
 *
 * When duplication is enabled, RSS options are ignored since every packet
 * is delivered to every ring.
 */
#define SNF_F_RX_DUPLICATE 0x300

/**
 * Each ring can have private references to a ring instead of shared
 * references.  This flag may yield better capture behavior when the
 * consumption rate of each rings varies enough to cause an important
 * number of packet drops.
 */
#define SNF_F_RX_PRIVATE 0x100


/**
 * Opens a board for sniffing and allocates a device handle.
 *
 * @brief Open device for single or multi-ring operation
 *
 * @param boardnum Boards are numbered from 0 to N-1 where 'N' is the
 *                 number of Myricom ports available on the system.
 *                 snf_getifaddrs() may be a useful utility to retrieve
 *                 the board number by interface name or mac address if
 *                 there are multiple
 *
 * @param num_rings Number of rings to allocate for receive-side scaling
 *                  feature, which determines how many different threads
 *                  can open their own ring via snf_ring_open().  If set
 *                  to 0 or less than zero, default value is used unless
 *                  SNF_NUM_RINGS is set in the environment.
 *
 * @param rss_parms Points to a user-initialized structure that selects
 *                  the RSS mechanism to apply to each incoming packet.
 *                  This parameter is only meaningful if there are more
 *                  than 1 rings to be opened.  By default, if users pass
 *                  a NULL value, the implementation will select its own
 *                  mechanism to divide incoming packets across rings.
 *                  RSS parameters are documented in @ref rss_options.
 *
 * @param dataring_sz Represents the total amount of memory to be used to
 *                    store incoming packet data for *all* rings to be
 *                    opened.  If the value is set to 0 or less than 0,
 *                    the library tries to choose a sensible default
 *                    unless SNF_DATARING_SIZE is set in the environment.
 *                    The value can be specified in megabytes (if it is
 *                    less than 1048576) or is otherwise considered to be
 *                    in bytes.  In either case, the library may slightly
 *                    adjust the user's request to satisfy alignment
 *                    requirements (typically 2MB boundaries).
 *
 * @param flags A mask of flags documented in @ref snf_open_flags.
 *
 * @param devhandle Device handle allocated if the call is successful
 *
 * @retval 0      Successful. the board is opened and a value devhandle is
 *                allocated (see remarks)
 * @retval EBUSY  Device is already opened
 * @retval EINVAL Invalid argument passed, most probably num_rings (if
 *                not, check syslog)
 * @retval E2BIG  Driver could not allocate requested dataring_sz (check
 *                syslog)
 * @retval ENOMEM Either library or driver did not have enough memory to
 *                allocate handle descriptors (but not data ring).
 *
 * @post If successful, the NIC switches from Ethernet mode to Capture mode
 *       and the Ethernet driver stops receiving packets.
 *
 * @post If successful, a call to @ref snf_start is required to the
 *       Sniffer-mode NIC to deliver packets to the host, and this call
 *       must occur after at least one ring is opened (@ref
 *       snf_ring_open).
 */
int snf_open(uint32_t boardnum,
             int num_rings,
             const struct snf_rss_params *rss_parms,
             int64_t dataring_sz,
             int flags,
             snf_handle_t *devhandle);


/**
 * Opens a board for sniffing and allocates a device handle using system
 * defaults.
 *
 * @brief Open device for single or multi-ring operation
 *
 * This function is a simplified version of @ref snf_open and ensures that
 * the resulting device is opened according to system defaults.  Since
 * the number of rings and flags can be set by module parameters, some
 * installations may prefer to control device-level parameters in a
 * system-wide configuration and keep the library calls simple.
 *
 * This call is equivalent to
 * @code
 * snf_open(boardnum, 0, NULL, 0, -1, devhandle);
 * @endcode
 *
 * @param boardnum Boards are numbered from 0 to N-1 where 'N' is the
 *                 number of Myricom ports available on the system.
 *                 snf_getifaddrs() may be a useful utility to retrieve
 *                 the board number by interface name or mac address if
 *                 there are multiple
 *
 * @param devhandle Device handle allocated if the call is successful
 *
 * @see snf_open
 */
int snf_open_defaults(uint32_t boardnum, snf_handle_t *devhandle);

/**
 * Start packet capture on a board.  Packet capture is only started if it
 * is currently stopped or not yet started for the first time.
 *
 * @param devhandle Device handle
 *
 * @remarks It is safe to restart packet capture via @ref snf_start and
 *          @ref snf_stop.
 * @remarks This call must be called before any packet can be received.
 */
int snf_start(snf_handle_t devhandle);

/**
 * Stop packet capture on a board.  This function should be used carefully
 * in multi-process mode as a single stop command stops packet capture on
 * all rings.  It is usually best to simply @ref snf_ring_close a ring to
 * stop capture on a ring.
 *
 * @param devhandle Device handle
 *
 * @remarks Stop instructs the NIC to drop all packets until the next @ref
 * snf_start() or until the board is closed.  The NIC only resumes
 * delivering packets when the board is closed, not when traffic is
 * stopped.
 */
int snf_stop(snf_handle_t devhandle);

/**
 * @brief Close board
 *
 * This function can be closed once all opened rings (if any) are closed
 * through @ref snf_ring_close.  Once a board is determined to be
 * closable, it is implicitly called as if a call had been previously made
 * to @ref snf_stop.
 *
 * @retval 0 Successful.
 * @retval EBUSY  Some rings are still opened and the board cannot be
 *                closed (yet).
 *
 * @post If successful, all resources allocated at open time are
 * unallocated and the device switches from Sniffer mode to Ethernet mode
 * such that the Ethernet driver resumes receiving packets.
 */
int snf_close(snf_handle_t devhandle);

/*!
 * Opaque snf ring handle, allocated at @ref snf_ring_open time when a
 * ring can be succesfully opened
 * @brief opaque snf ring handle */
typedef struct snf_ring   *snf_ring_t;

/**
 * Opens the next available ring
 *
 * @param devhandle Device handle, obtained from a successful call to 
 *               @ref snf_open
 * @param ringh Ring handle allocated if the call is successful.
 *
 * @retval 0 Successful. The ring is opened and ringh contains the ring
 *           handle.
 * @retval EBUSY Too many rings already opened
 *
 * @post If successful, a call to @ref snf_start is required to the
 *       Sniffer-mode NIC to deliver packets to the host.
 */
int snf_ring_open(snf_handle_t devhandle, snf_ring_t *ringh);

/**
 * Opens a ring from an opened board.
 *
 * @param devhandle Device handle, obtained from a successful call to 
 *               @ref snf_open
 * @param ring_id Ring number to open, from 0 to @b num_rings - 1.  If
 *                the value is -1, this function behaves as if 
 *                @ref snf_ring_open was called.
 * @param ringh Ring handle allocated if the call is successful.
 *
 * @retval 0 Successful. The ring is opened and ringh contains the ring
 *           handle.
 * @retval EBUSY If ring_id == -1, Too many rings already opened.
 *               If ring_id >= 0, that ring is already opened.
 *
 * @post If successful, a call to @ref snf_start is required to the
 *       Sniffer-mode NIC to deliver packets to the host.
 */
int snf_ring_open_id(snf_handle_t devhandle, int ring_id, snf_ring_t *ringh);

/**
 * Close a ring
 *
 * This function is used to inform the underlying device that no further
 * calls to @ref snf_ring_recv will be made.  If the device is not
 * subsequently closed (@ref snf_close), all packets that would have been
 * delivered to this ring are dropped.  Also, by calling this function,
 * users confirm that all packet processing for packets obtained on this
 * ring via @ref snf_ring_recv is complete.
 *
 * @param ringh Ring handle
 *
 * @retval 0 Successful.
 *
 * @post The user has processed the last packet obtained with 
 *       @ref snf_ring_recv and the device can safely be closed via 
 *       @ref snf_close if all other rings are also closed.
 */
int snf_ring_close(snf_ring_t ringh);

/**
 * Structure to describe a packet received on a data ring.
 */
struct snf_recv_req {
  void    *pkt_addr;  /**< Pointer to packet directly in data ring */
  uint32_t length;    /**< Length of packet, does not include Ethernet CRC */
  uint64_t timestamp; /**< 64-bit timestamp in nanoseconds */
  uint64_t token;     /**< Internally reserved token, can be ignored */
};

/**
 * @brief Receive next packet from a receive ring.
 *
 * This function is used to return the next available packet in a receive
 * ring.  The function can block indefinitely, for a specific timeout or
 * be used as a non-blocking call with a timeout of 0.
 *
 * @param ringh Ring handle (from @ref snf_ring_open)
 * @param timeout_ms Receive timeout to control how the function blocks
 * for the next packet. If the value is less than 0, the function can
 * block indefinitely.  If the value is 0, the function is guaranteed to
 * never enter a blocking state and returns EAGAIN unless there is a
 * packet waiting.  If the value is greater than 0, the caller inidicates
 * a desired wait time in milliseconds.  With a non-zero wait time, the
 * function only blocks if there are no outstanding packets.  If the
 * timeout expires before a packet can be received, the function returns
 * EAGAIN (and @b not ETIMEDOUT).  In all cases, users should expect that
 * the function may return EINTR as the result of signal delivery.
 * @param recv_req Receive Packet structure, only updated when a the
 *                 function returns 0 for a successful packet receive
 *                 (@ref snf_recv_req)
 *
 * @retval 0 Successful packet delivery, recv_req is updated with packet
 *           information.
 * @retval EINTR The call was interrupted by a signal handler
 * @retval EAGAIN No packets available (only when timeout is >= 0).
 *
 * @remarks The packet returned always points directly into the receive
 * ring where the NIC has DMAed the packet (there are no copies).  As
 * such, the user obtains a pointer to library/driver allocated memory.
 * Users can modify the contents of the packets but should remain within
 * the boundaries of @b pkt_addr and @b length.
 *
 * @remarks Upon calling the function, the library assumes that the user
 * is done processing the previous packet.  The same assumption is made
 * when the ring is closed (@ref snf_ring_close).
 */
int snf_ring_recv(snf_ring_t ringh, int timeout_ms, struct snf_recv_req *recv_req);

/**
 * Structure to return statistics from a ring.  The Hardware-specific
 * counters apply to all rings as they are counted before any
 * demultiplexing to a ring is applied.
 */
struct snf_ring_stats {
  uint64_t  nic_pkt_recv;       /**< Number of packets received by Hardware Interface */
  uint64_t  nic_pkt_overflow;   /**< Number of packets dropped by Hardware Interface */ 
  uint64_t  nic_pkt_bad;        /**< Number of Bad CRC/PHY packets seen by Hardware Interface */
  uint64_t  ring_pkt_recv;      /**< Number of packets received into the receive ring */
  uint64_t  ring_pkt_overflow;  /**< Number of packets dropped because of insufficient space in receive ring */
  uint64_t  nic_bytes_recv;     /**< Number of raw bytes received by the Hardware Interface on 
                                     all rings. Each Ethernet data packet includes 8 bytes of HW 
                                     header, 4 bytes of CRC and the result is aligned to 16 bytes 
                                     such that a minimum size 60 byte packet counts for 80 bytes.  */
  uint64_t  snf_pkt_overflow;   /**< Number of packets dropped because of insufficient space in shared
                                     SNF buffering */
};

/** 
 * @brief Get statistics from a receive ring
 *
 * @param ringh Ring handle
 * @param stats User-provided pointer to a statistics structure 
 *              @ref snf_ring_stats, filled in by the library.
 *
 * @remarks This call is provided as a convenience and should not be
 * relied on for time-critical applications or for high levels of
 * accuracy.  Statistics are only updated by the NIC periodically.
 *
 * @warning Administrative clearing of NIC counters while a Sniffer-based
 * application is running may cause some of the counters to be incorrect.
 */
int snf_ring_getstats(snf_ring_t ringh, struct snf_ring_stats *stats);


/*! @defgroup injection Packet injection
 *
 * SNF Packet injection routines that can be used for independent packet
 * generation or coupled to reinject packets received with
 * @ref snf_ring_recv.
 *
 * @{ */
/*!
 * Opaque injection handle, allocated by @ref snf_inject_open.
 * There are only a limited amount of injection handles per NIC/board.
 */
typedef struct snf_inject_handle *snf_inject_t;

/**
 * @brief Open a board for injection and allocate an injection handle
 *
 * @param boardnum Boards are numbered from 0 to N-1 where 'N' is the
 *                 number of Myricom ports available on the system.
 *                 snf_getifaddrs() may be a useful utility to retrieve
 *                 the board number by interface name or mac address if
 *                 there are multiple
 *
 * @param flags Flags for injection handle. None are currently defined.
 *
 * @param handle  Injection handle allocated if the call is successful.
 *
 *
 * @retval 0 Success. An injection handle is opened and allocated.
 * @retval EBUSY Ran out of injection handles for this board
 * @retval ENOMEM Ran out of memory to allocate new injection handle
 */
int snf_inject_open(int boardnum, int flags, snf_inject_t *handle);

/**
 * @brief Send a packet and optionally block until send resources are
 *        available.
 *
 * This send function is optimized for high packet rate injection.  While
 * it can be coupled with a receive ring to reinject a packet, it is not
 * strictly necessary.  This function can be used as part of a packet
 * generator.  When the function returns successfully, the packet is
 * guaranteed to be completely buffered by SNF: no references are kept to
 * the input data and the caller is free to safely modify its contents.
 * A successful return does not, however, guarantee that the packet has
 * been injected into the network.  The SNF implementation may chose to
 * hold on to the packet for coalescing in order to improve packet
 * throughput.  
 *
 * @param inj Injection handle
 * @param timeout_ms Timeout in milliseconds to wait if insufficient send
 *        resources are available to inject a new packet.  Insufficient
 *        resources can be a lack of send descriptors or a full send queue
 *        ring.  If timeout_ms is 0, the function won't block for send
 *        resources and returns EAGAIN.
 * @param pkt Pointer to the packet to be sent.  The packet must be a
 *        pointer to a complete Ethernet frame (without the trailing CRC)
 *        and start with a valid Ethernet header.  The hardware will
 *        append 4-CRC bytes at the end of the packet.  The maximum valid
 *        packet size is 9000 bytes and is enforced by the library.  The
 *        minimum valid packet size is 60 bytes, although any packet
 *        smaller than 60 bytes will be accepted by the library and padded
 *        by the hardware.
 * @param length The length of the packet, excluding the trailing 4 CRC
 *        bytes.
 *
 * @retval 0 Successful. The packet is buffered by SNF.
 * @retval EAGAIN Insufficient resources to send packet.  If timeout_ms is
 *                non-zero, the caller will have blocked at least that
 *                many milliseconds before resources could become
 *                available.
 * @retval EINVAL Packet length is larger than 9000 bytes.
 *
 * @post If succcessful, the packet is completely buffered for send by
 *       SNF.  The implementation guarantees that it will eventually send
 *       the packet out in a timely fashion without requiring further
 *       calls into SNF.
 */
int snf_inject_send(snf_inject_t inj, int timeout_ms, const void *pkt, uint32_t length);

/**
 * @brief Close injection handle
 *
 * This function closes an injection handle and ensures that all pending
 * sends are sent by the NIC.
 *
 * @param inj Injection handle 
 *
 * @retval 0 Successful
 *
 * @post Once closed, the injection handle will have ensured that any
 *       pending sends have been sent out on the wire.  The handle is then
 *       made available again for the underlying board's limited amount of
 *       handles.
 */
int snf_inject_close(snf_inject_t inj);

/**
 * Structure to return statistics from an injection handle.  The
 * hardware-specific counters (nic_) apply to all injection handles.
 */
struct snf_inject_stats {
  uint64_t  inj_pkt_send;      /**< Number of packets sent by this injection endpoint */
  uint64_t  nic_pkt_send;       /**< Number of total packets sent by Hardware Interface */
  uint64_t  nic_bytes_send;     /**< Number of raw bytes sent by Hardware Interface (see nic_bytes_recv) */
};

/**
 * @brief Get statistics from an injection handle
 *
 * @param inj Injection Handle
 * @param stats User-provided pointer to a statistics structure 
 *        @ref snf_inject_stats, filled in by the SNF implementation.
 *
 * @remarks This call is provided as a convenience and should not be
 * relied on for time-critical applications or for high levels of
 * accuracy.  Statistics are only updated by the NIC periodically.
 *
 * @warning Administrative clearing of NIC counters while a Sniffer-based
 * application is running may cause some of the counters to be incorrect.
 */
int snf_inject_getstats(snf_inject_t inj, struct snf_inject_stats *stats);
/*! @} injection */

/** @defgroup reflect Packet reflect to netdev (kernel stack)
 *
 * SNF Packets can be reflected to the network device associated with the
 * sniffer handle such that these packets propagate through the kernel's
 * network stack (as if they came from the NIC).  The application can
 * arbitrate packets received with @ref snf_ring_recv and forward or
 * "reflect" some packets to the network device for processing by the host
 * stack.
 *
 * @{ */
/**
 * Opaque handle returned by @ref snf_netdev_reflect_enable and used
 * to reflect packets onto by @ref snf_netdev_reflect.
 */
typedef void * snf_netdev_reflect_t;

/**
 * @brief Enable a network device for packet reflection.
 *
 * @param hsnf handle for network device to reflect onto, obtained by @ref snf_open
 *
 * @param handle  Reflection handle.
 *
 *
 * @retval 0 Success. An reflection handle is enabled.
 */
int snf_netdev_reflect_enable(snf_handle_t hsnf, snf_netdev_reflect_t *handle);

/**
 * @brief Reflect a packet to the network device.
 *
 * @param  ref_dev Reflection handle
 * @param pkt Pointer to the packet to be reflected to the network device.
 *        The packet must be a pointer to a complete Ethernet frame (without
 *        the trailing CRC) and start with a valid Ethernet header. 
 * @param length The length of the packet, excluding the trailing 4 CRC
 *        bytes.
 *
 * @retval 0 Successful. The packet is buffered by SNF.
 *
 * @post If succcessful, the packet is completely buffered into the network
 *       device recieve path.
 */
int snf_netdev_reflect(snf_netdev_reflect_t ref_dev, const void *pkt, uint32_t length);



#ifdef __cplusplus
}
#endif
#endif /* _snf_h */
