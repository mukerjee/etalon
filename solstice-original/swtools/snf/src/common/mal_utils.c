/*************************************************************************
 * The contents of this file are subject to the MYRICOM MYRINET          *
 * EXPRESS (MX) NETWORKING SOFTWARE AND DOCUMENTATION LICENSE (the       *
 * "License"); User may not use this file except in compliance with the  *
 * License.  The full text of the License can found in LICENSE.TXT       *
 *                                                                       *
 * Software distributed under the License is distributed on an "AS IS"   *
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied.  See  *
 * the License for the specific language governing rights and            *
 * limitations under the License.                                        *
 *                                                                       *
 * Copyright 2003 - 2009 by Myricom, Inc.  All rights reserved.          *
 *************************************************************************/

#include "mal.h"
#include "mal_io.h"
#include "mal_valgrind.h"

#ifdef MAL_KERNEL
#error Cannot compile mal_utils.c into kernel
#endif

#if !MAL_OS_WINDOWS
#if MAL_OS_MACOSX || MAL_OS_FREEBSD
# include <sys/sysctl.h>
# include <sys/socket.h>
# include <net/if_dl.h>
# include <net/route.h>
#endif
#include <string.h>
#include <netinet/in.h>
#include <net/if.h>
#else /* MAL_OS_WINDOWS */
#include <winsock2.h>
#include <ws2ipdef.h>
#include <iphlpapi.h>
#endif

#ifdef HAVE_SYS_SOCKIO_H
#  include <sys/sockio.h>
#endif

#ifdef HAVE_IFADDRS_H
#  include <ifaddrs.h>
#endif

#if MAL_OS_LINUX
static
int
get_macaddr(const char *ifname, uint8_t mac_addr[6])
{
  struct ifreq ifr;
  int fd;
  int rc = 0;
  
  if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    rc = errno;
  else {
    strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
    if (ioctl(fd, SIOCGIFHWADDR, &ifr))
      rc = errno;
    else
      memcpy(mac_addr, ifr.ifr_hwaddr.sa_data, 6);
    close(fd);
  }
  return rc;
}
#elif MAL_OS_SOLARIS /* Solaris */
#include <net/if_arp.h>
static
int
get_macaddr(const char *ifname, uint8_t mac_addr[6])
{
  struct ifreq ifr;
  struct xarpreq ar;
  int fd;
  int rc = 0;

  if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    rc = errno;
  else {
    strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
    if (ioctl(fd, SIOCGIFADDR, &ifr))
      rc = errno;
    else {
      memset(&ar, 0, sizeof (ar));
      memcpy(&ar.xarp_pa, &ifr.ifr_addr, sizeof(ifr.ifr_addr));
      ar.xarp_ha.sdl_family = AF_LINK;
      if (ioctl(fd, SIOCGXARP, (caddr_t)&ar) < 0)
        rc = errno;
      else if ((ar.xarp_flags & ATF_AUTHORITY) == 0)
        rc = ENODEV;
      else
        memcpy(mac_addr, (uint8_t *)LLADDR(&ar.xarp_ha), 6);
    }
    close(fd);
  }
  return rc;
}
#elif MAL_OS_MACOSX || MAL_OS_FREEBSD
static
int
get_macaddr(const char *ifname, uint8_t mac_addr[6])
{
  struct if_msghdr *ifm;
  struct sockaddr_dl *sdl;
  size_t mib_len;
  char *buf, *p;
  uint8_t *macp = NULL;
  int rc = 0;
  int mib[] = { CTL_NET, AF_ROUTE, 0, AF_LINK, NET_RT_IFLIST, 0 };
  size_t ifname_len, len;

  if (sysctl(mib, 6, NULL, &mib_len, NULL, 0) < 0)
    rc = errno;
  else {
    ifname_len = strlen(ifname);
    if ((buf = mal_malloc(mib_len)) == NULL)
      rc = ENOMEM;
    else {
      if (sysctl(mib, 6, buf, &mib_len, NULL, 0) < 0) {
	rc = errno;
	mal_free(buf);
	return rc;
      }
      for (p = buf; !macp && p < buf + mib_len; p += ifm->ifm_msglen) {
        ifm = (struct if_msghdr *)p;
        sdl = (struct sockaddr_dl *)(ifm + 1);
        if (ifm->ifm_type != RTM_IFINFO || (ifm->ifm_addrs & RTA_IFP) == 0)
          continue;
        if (sdl->sdl_family != AF_LINK || !sdl->sdl_nlen)
          continue;
        len = sdl->sdl_nlen > ifname_len ? sdl->sdl_nlen : ifname_len;
        if (memcmp(sdl->sdl_data, ifname, len) == 0)
          macp = (uint8_t *)LLADDR(sdl);
      }
      if (macp)
        memcpy(mac_addr, macp, 6);
      else
        rc = ENODEV;
      mal_free(buf);
    }
  }
  return rc;
}
#elif MAL_OS_WINDOWS
/* Returns non-zero if ipaddress matches any of the strings in
 * ip_addr_string. */
static
int
ipaddress_matches_ip_addr_string(const char *ipaddress,
				 PIP_ADDR_STRING ip_addr_string)
{
  int ret;

  ret = 0;

  while (ip_addr_string) {
    if (strcmp(ipaddress, ip_addr_string->IpAddress.String) == 0) {
      ret = 1;
      break;
    }

    ip_addr_string = ip_addr_string->Next;
  }

  return ret;
}

/* Return a pointer to the adaper info that matches ifname. NULL if not
 * found. Normally ifname is something like eth0 or myri0 but on Windows
 * it's an undecipherable GUID. Rather than deal with that mess we use
 * the convention that ifname will be the string form of the dotted ip
 * address. */
static
PIP_ADAPTER_INFO
lookup_ifname_in_adapter_info(const char *ifname, PIP_ADAPTER_INFO info)
{
  while (info != NULL) {
    if (ipaddress_matches_ip_addr_string(ifname, &info->IpAddressList)) {
      break;
    }

    info = info->Next;
  }

  return info;
}

/* Allocate and return infromation on all the adapters. Returns NULL if
 * unable to for any reason. Caller is responsible for freeing the
 * memory when done. */
static
PIP_ADAPTER_INFO
get_adapters_info(void)
{
  PIP_ADAPTER_INFO info;
  DWORD dw;
  ULONG buf_len;

  info = NULL;

  buf_len = 0;
  dw = GetAdaptersInfo(NULL, &buf_len);
  if (dw == ERROR_BUFFER_OVERFLOW) {
    info = mal_malloc(buf_len);
    if (info != NULL) {
      dw = GetAdaptersInfo(info, &buf_len);
      if (dw != ERROR_SUCCESS) {
	mal_free(info);
	info = NULL;
      }
    }
  }

  return info;
}

static
int
get_macaddr(const char *ifname, uint8_t mac_addr[6])
{
  PIP_ADAPTER_INFO info;
  PIP_ADAPTER_INFO matched_info;
  int ret;

  ret = -1;

  info = get_adapters_info();
  matched_info = lookup_ifname_in_adapter_info(ifname, info);
  if (matched_info != NULL && matched_info->AddressLength == 6) {
    memcpy(mac_addr, matched_info->Address, 6);
    ret = 0;
  }

  if (info != NULL) {
    mal_free(info);
  }

  return ret;
}
#elif MAL_OS_UDRV
static
int
get_macaddr(const char *ifname, uint8_t mac_addr[6])
{
  return ENODEV;
}
#else
#warning get_macaddr() not implemented on this platform
#endif

static
int
mal_match_macaddr(const char *ifname,
                  const struct mal_nic_info *nic_info, int num_nics)
{
  int i;
  uint64_t nic_id;
  uint8_t mac_addr[6];

  if (get_macaddr(ifname, mac_addr))
    return -1;
  mal_macaddr_to_nic_id(mac_addr, &nic_id);

  for (i = 0; i < num_nics; i++) {
    if (nic_info[i].nic_id == nic_id && MAL_IS_ETHER_BOARD(nic_info[i].nic_type))
	return i;
  }
  return -1;
}

int
mal_getifaddrs(struct mal_ifaddrs **ifaddrs_o, int inet_only, int *num_ifs)
{
  struct mal_ifaddrs *ifaddrs_mal = NULL;
  int num_if_myri = 0, num_if_mal = 0;
  int i, rc = 0;
  struct mal_nic_info *nic_info = NULL, *ni;

  if ((rc = mal_get_nicinfo(&nic_info, &num_if_myri)) || !num_if_myri)
    goto bail;

#if HAVE_IFADDRS_H
  /* Get list of all addresses on this node */
  {
    struct ifaddrs *ifaddr, *ifa;
    int idx;

    if (getifaddrs(&ifaddr) == -1) {
      rc = errno;
      goto bail;
    }

    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
      ifa->ifa_data = NULL;
      
      /* make sure there is an address field */
      if (ifa->ifa_addr == NULL) continue;

      if (inet_only && ifa->ifa_addr->sa_family != AF_INET)
        ;
      else if ((idx = mal_match_macaddr(ifa->ifa_name,
                      nic_info, num_if_myri)) >= 0) {
        num_if_mal++;
        ifa->ifa_data = &nic_info[idx];
      }
    }

    if (num_if_mal) {
      struct sockaddr_in *sin;
      char *ifname;
      ifaddrs_mal = mal_malloc((sizeof(struct mal_ifaddrs)+IFNAMSIZ) * num_if_mal);
      if (ifaddrs_mal == NULL) {
        rc = ENOMEM;
      }
      else {
        ifname = (char *)(ifaddrs_mal+num_if_mal);
        for (ifa = ifaddr, i = 0; ifa != NULL; ifa = ifa->ifa_next) {
          ni = (struct mal_nic_info *) ifa->ifa_data;
          if (ni == NULL)
            continue;
          ifaddrs_mal[i].mal_ifa_next = &ifaddrs_mal[i+1];
          strncpy(ifname, ifa->ifa_name, IFNAMSIZ);
          ifname[IFNAMSIZ-1] = '\0';
          ifaddrs_mal[i].mal_ifa_name = ifname;
          ifname += IFNAMSIZ;
          ifaddrs_mal[i].mal_ifa_flags = 0;
          if (ifa->ifa_flags & IFF_BROADCAST)
            ifaddrs_mal[i].mal_ifa_flags |= MAL_IFA_FLAG_BROADCAST;
          if (ifa->ifa_flags & IFF_MULTICAST)
            ifaddrs_mal[i].mal_ifa_flags |= MAL_IFA_FLAG_MULTICAST;
          sin = (struct sockaddr_in *) ifa->ifa_addr;
          if (sin)
            ifaddrs_mal[i].mal_ifa_ipaddr = sin->sin_addr.s_addr;
          sin = (struct sockaddr_in *) ifa->ifa_netmask;
          if (sin)
            ifaddrs_mal[i].mal_ifa_netmask = sin->sin_addr.s_addr;
          sin = (struct sockaddr_in *) ifa->ifa_broadaddr;
          if (sin)
            ifaddrs_mal[i].mal_ifa_bcastaddr = sin->sin_addr.s_addr;
          ifaddrs_mal[i].mal_ifa_boardnum = ni->boardnum;
          ifaddrs_mal[i].mal_ifa_nicid = ni->nic_id;
          mal_nic_id_to_macaddr(ni->nic_id, ifaddrs_mal[i].mal_ifa_macaddr);
          ifaddrs_mal[i].mal_ifa_max_endpoints = ni->nic_max_endpoints;
          i++;
        }
        ifaddrs_mal[i-1].mal_ifa_next = NULL;
      }
    }
    freeifaddrs(ifaddr);
  }
#elif MAL_OS_SOLARIS
#define _lifr_to_u32(lifrp) (uint32_t)(  \
	    ((struct sockaddr_in *) &((lifrp)->lifr_addr))->sin_addr.s_addr)
  {
    struct lifnum lifn;
    struct lifconf lifc;
    struct lifreq *lifrp = NULL;
    uint64_t nic_id;
    int j, fd, idx;
    int num_if_lifc;
    struct mal_ifaddrs *ifm;
    size_t slen;
    char *ifname;
    uint8_t mac_addr[6];
    struct mal_nic_info **idx_map_nic = NULL;

    if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
      rc = errno;
    else {
      lifn.lifn_family = AF_UNSPEC;
      lifn.lifn_flags = 0;
      if (ioctl(fd, SIOCGLIFNUM, &lifn))
        lifn.lifn_count = 16;
      else
        lifn.lifn_count += 16;

      for (;;) {
        lifc.lifc_len = lifn.lifn_count * sizeof(*lifrp);
        lifrp = mal_malloc(lifc.lifc_len);
        if (lifrp == NULL) {
          rc = ENOMEM;
          goto bail;
        }
        lifc.lifc_family = AF_UNSPEC;
        lifc.lifc_flags = 0;
        lifc.lifc_buf = (char *)lifrp;
        if (ioctl(fd, SIOCGLIFCONF, &lifc)) {
          mal_free(lifrp);
          if (errno == EINVAL) {
            lifn.lifn_count <<= 1;
            continue;
          }
          close(fd);
          rc = errno;
          goto bail;
        }
        if (lifc.lifc_len < (lifn.lifn_count - 1) * sizeof(*lifrp))
          break;
        mal_free(lifrp);
        lifn.lifn_count <<= 1;
      }
      /* lifrp contains interfaces */
      num_if_lifc = lifc.lifc_len / sizeof(*lifrp);
      if (!(idx_map_nic = mal_calloc(num_if_lifc, sizeof(struct mal_nic_info *)))) {
        mal_free(lifrp);
        rc = ENOMEM;
        goto bail;
      }

      for (i = 0; i < num_if_lifc; i++) {
        idx_map_nic[i] = NULL;
        if ((idx = mal_match_macaddr(lifrp[i].lifr_name, 
                      nic_info, num_if_myri)) >= 0) {
          idx_map_nic[i] = &nic_info[idx];
          num_if_mal++;
        }
      }

      if (!num_if_mal)
	goto bail;

      ifm = ifaddrs_mal = 
        mal_calloc(1, (sizeof(struct mal_ifaddrs)+IFNAMSIZ) * num_if_mal);
      if (ifaddrs_mal == NULL) {
        mal_free(lifrp);
        mal_free(idx_map_nic);
        rc = ENOMEM;
        goto bail;
      }
      ifname = (char *)(ifaddrs_mal+num_if_mal);
      for (i = 0; i < num_if_lifc; i++) {
        ni = (struct mal_nic_info *) idx_map_nic[i];
        if (ni == NULL)
          continue;
        ifm->mal_ifa_next = ifm + 1;
        strncpy(ifname, lifrp[i].lifr_name, IFNAMSIZ);
        ifname[IFNAMSIZ-1] = '\0';
        ifm->mal_ifa_name = ifname;
        ifname += IFNAMSIZ;
        ifm->mal_ifa_flags = 0;
        if (!ioctl(fd, SIOCGLIFFLAGS, lifrp)) {
          if (lifrp[i].lifr_flags & IFF_BROADCAST)
            ifm->mal_ifa_flags |= MAL_IFA_FLAG_BROADCAST;
          if (lifrp[i].lifr_flags & IFF_MULTICAST)
            ifm->mal_ifa_flags |= MAL_IFA_FLAG_MULTICAST;
        }
        if (!ioctl(fd, SIOCGLIFADDR, &lifrp[i]))
          ifm->mal_ifa_ipaddr = _lifr_to_u32(&lifrp[i]);
        if (!ioctl(fd, SIOCGLIFNETMASK, &lifrp[i]))
          ifm->mal_ifa_netmask = _lifr_to_u32(&lifrp[i]);
        if (ifm->mal_ifa_flags & MAL_IFA_FLAG_BROADCAST)
          if (!ioctl(fd, SIOCGLIFBRDADDR, &lifrp[i]))
            ifm->mal_ifa_bcastaddr = _lifr_to_u32(&lifrp[i]);
        ifm->mal_ifa_boardnum = ni->boardnum;
        ifm->mal_ifa_nicid = ni->nic_id;
        mal_nic_id_to_macaddr(ni->nic_id, ifm->mal_ifa_macaddr);
        ifm->mal_ifa_max_endpoints = ni->nic_max_endpoints;
        ifm++;
      }
      if (ifm != ifaddrs_mal)
	ifm[-1].mal_ifa_next = NULL;
      mal_free(lifrp);
      mal_free(idx_map_nic);
      close(fd);
    }
  }
#elif MAL_OS_WINDOWS
  {
    INTERFACE_INFO if_info[32];
    SOCKET s;
    DWORD bytes_returned;
    int num_if;
    int loop;
    struct sockaddr_in *psin;
    IN_ADDR in_addr;
    char *ifname;
    char *tmp_ifname;
    int idx;
    unsigned long ip_addr, netmask, broadcast_addr;

    /* Populate if_info. */
    s = WSASocket(AF_INET, SOCK_DGRAM, 0, 0, 0, 0);
    if (s == INVALID_SOCKET) {
      rc = -1;
      goto bail;
    }
    if (WSAIoctl(s, SIO_GET_INTERFACE_LIST, NULL, 0, &if_info,
		 sizeof (if_info), &bytes_returned, NULL, NULL) ==
	SOCKET_ERROR) {
      closesocket(s);
      rc = -1;
      goto bail;
    }
    closesocket(s);
    num_if = bytes_returned / sizeof (INTERFACE_INFO);

    /* Count the number of mal compatible interfaces. */
    for (loop = 0; loop < num_if; ++loop) {
      psin = (SOCKADDR_IN*)&if_info[loop].iiAddress;
      in_addr.S_un.S_addr = psin->sin_addr.S_un.S_addr;
      if ((idx = mal_match_macaddr(inet_ntoa(in_addr), nic_info,
				   num_if_myri)) >= 0) {
	num_if_mal++;
      }
    }

    if (num_if_mal == 0) {
      /* rc is not set because zero interfaces is not an error. */
      goto bail;
    }

    ifaddrs_mal = mal_malloc((sizeof (struct mal_ifaddrs) + 16) *
			     num_if_mal);
    if (ifaddrs_mal == NULL) {
      rc = ENOMEM;
      goto bail;
    }

    i = 0;
    ifname = (char*)(ifaddrs_mal+num_if_mal);
    for (loop = 0; loop < num_if; ++loop) {
      psin = (SOCKADDR_IN*)&if_info[loop].iiAddress;
      ip_addr = psin->sin_addr.S_un.S_addr;
      in_addr.S_un.S_addr = psin->sin_addr.S_un.S_addr;
      tmp_ifname = inet_ntoa(in_addr);
      if ((idx = mal_match_macaddr(tmp_ifname, nic_info,
				   num_if_myri)) >= 0) {
	ifaddrs_mal[i].mal_ifa_next = &ifaddrs_mal[i+1];
	strcpy(ifname, tmp_ifname);
	ifaddrs_mal[i].mal_ifa_name = ifname;
	ifname += 16;
	ifaddrs_mal[i].mal_ifa_flags = 0;
	if (if_info[loop].iiFlags & IFF_BROADCAST) {
	  ifaddrs_mal[i].mal_ifa_flags |= MAL_IFA_FLAG_BROADCAST;
	}
	if (if_info[loop].iiFlags & IFF_MULTICAST) {
	  ifaddrs_mal[i].mal_ifa_flags |= MAL_IFA_FLAG_MULTICAST;
	}
	ifaddrs_mal[i].mal_ifa_ipaddr = psin->sin_addr.S_un.S_addr;
	psin = (SOCKADDR_IN*)&if_info[loop].iiNetmask;
	netmask = psin->sin_addr.S_un.S_addr;
	ifaddrs_mal[i].mal_ifa_netmask = psin->sin_addr.S_un.S_addr;
	/*
	  iiBroadcastAddress seems to be broken. Calculate it ourselves.
	psin = (SOCKADDR_IN*)&if_info[loop].iiBroadcastAddress;
	ifaddrs_mal[i].mal_ifa_bcastaddr = psin->sin_addr.S_un.S_addr;
	*/
	broadcast_addr = ip_addr | ~netmask;
	ifaddrs_mal[i].mal_ifa_bcastaddr = broadcast_addr;
	ifaddrs_mal[i].mal_ifa_boardnum = nic_info[idx].boardnum;
	ifaddrs_mal[i].mal_ifa_nicid = nic_info[idx].nic_id;
	mal_nic_id_to_macaddr(nic_info[idx].nic_id,
			      ifaddrs_mal[i].mal_ifa_macaddr);
        ifaddrs_mal[i].mal_ifa_max_endpoints = nic_info[idx].nic_max_endpoints;
	i++;
      }
    }
    ifaddrs_mal[i-1].mal_ifa_next = NULL;
  }
#else
  fprintf(stderr, "mal_getifaddrs() not available\n");
  rc = ENXIO;
#endif

bail:
  if (!rc) {
    *ifaddrs_o = ifaddrs_mal;
    *num_ifs = num_if_mal;
  }
  if (nic_info != NULL)
    mal_free_nicinfo(nic_info);

  return rc;
}

void
mal_freeifaddrs(struct mal_ifaddrs *ifaddrs)
{
  mal_free(ifaddrs);
}
