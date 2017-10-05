#define _GNU_SOURCE
#include <dlfcn.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/ioctl.h>
#include <net/if.h>

#define LOCAL_CTRL_DEVNAME "eth3"
#define SWITCH_CTRL_IP "10.10.2.1"
#define SWITCH_CTRL_PORT "8123"

struct traffic_info {
  char src[INET_ADDRSTRLEN];
  char dst[INET_ADDRSTRLEN];
  size_t size;
};

struct traffic_info* info;
char local_ip[NI_MAXHOST];
int ctrl_sock = -1;

ssize_t write(int fd, void *buffer, size_t size);
int socket(int domain, int type, int protocol);
int close(int sockfd);

static int (*next_socket)(int domain, int type, int protocol) = NULL;
static int (*next_send)(int socket, const void *buffer, size_t length, int flags) = NULL;
static int (*next_write)(int fd, void *buffer, size_t size) = NULL;
static int (*next_close)(int sockfd) = NULL;

void get_local_ip(int fd, char* saddr) {
  struct ifreq ifr;

  ifr.ifr_addr.sa_family = AF_INET;
  strncpy(ifr.ifr_name, LOCAL_CTRL_DEVNAME, IFNAMSIZ-1);
  ioctl(fd, SIOCGIFADDR, &ifr);

  strncpy(saddr, inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr), INET_ADDRSTRLEN);
}

void get_remote_ip(int fd, char* raddr) {
  socklen_t len;
  struct sockaddr_storage addr;

  len = sizeof addr;
  getpeername(fd, (struct sockaddr*)&addr, &len);

  // deal with both IPv4 and IPv6:
  if (addr.ss_family == AF_INET) {
    struct sockaddr_in *s = (struct sockaddr_in *)&addr;
    inet_ntop(AF_INET, &s->sin_addr, raddr, INET_ADDRSTRLEN);
  }
}

static void open_ctrl_socket() {
  if(ctrl_sock == -1){
    struct addrinfo hints, // Used to provide hints to getaddrinfo()
                    *res,  // Used to return the list of addrinfo's
                    *p;    // Used to iterate over this list

    char *host=SWITCH_CTRL_IP, *port=SWITCH_CTRL_PORT;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM; //TCP

    if (getaddrinfo(host, port, &hints, &res) != 0) {
      perror("getaddrinfo() failed");
      exit(-1);
    }

    for(p = res; p != NULL; p = p->ai_next)  {
      if ((ctrl_sock = next_socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
	perror("Could not open socket");
	continue;
      }

      if (connect(ctrl_sock, p->ai_addr, p->ai_addrlen) == -1)  {
	next_close(ctrl_sock);
	perror("Could not connect to socket");
	continue;
      }

      break;
    }
    freeaddrinfo(res);
  }
}

// Find the library version of the function that we are wrapping
static void get_next_fn(void** next_fn, char* fn) {
  char* msg;

  if(! *next_fn){
    *next_fn = dlsym(RTLD_NEXT, fn);
    if ((msg = dlerror()) != NULL) {
      fprintf(stderr, "dlopen failed on %s: %s\n", fn, msg);
      exit(1);
    } else {
      /* fprintf(stderr,  "next_%s = %p\n", fn, *next_fn); */
    }
  }
}

// Wrap up the socket() call
int socket(int domain, int type, int protocol) {
  char* fn_name = "socket";
  get_next_fn((void**)&next_socket,fn_name);

  open_ctrl_socket();

  int sockfd = next_socket(domain, type, protocol);

  info = (struct traffic_info *)malloc(sizeof(struct traffic_info));
  memset(info, 0, sizeof(struct traffic_info));

  get_local_ip(sockfd, info->src);
  /* fprintf(stderr, "LOCAL_IP: %s\n", info->src); */

  return sockfd;
}

// Wrap up the write() call
ssize_t write(int fd, void *buffer, size_t size) {
  ssize_t nbytes = -1;
  char* fn_name = "write";
  get_next_fn((void**)&next_write,fn_name);

  //Get the destination address
  if (fd != ctrl_sock) {
    get_remote_ip(fd, info->dst);
    info->size = size;
    nbytes = next_write(ctrl_sock, info, sizeof(struct traffic_info));

    if (nbytes != sizeof(struct traffic_info)){
      fprintf(stderr, "Failed to send ctrl message\n");
      return nbytes;
    }
  /* fprintf(stderr, "SIZE: %ld\n", size); */
  }
  return next_write(fd, buffer, size);
}

// Wrap up the send() call
ssize_t send(int fd, const void *buffer, size_t size, int flags) {
  ssize_t nbytes = -1;
  char* fn_name = "send";
  get_next_fn((void**)&next_send,fn_name);

  //Get the destination address
  if (fd != ctrl_sock) {
    get_remote_ip(fd, info->dst);
    info->size = size;
    nbytes = next_send(ctrl_sock, info, sizeof(struct traffic_info), flags);
    if (nbytes != sizeof(struct traffic_info)){
      fprintf(stderr, "Failed to send ctrl message\n");
      return nbytes;
    }
  }
  return next_send(fd, buffer, size, flags);
}

// Wrap up the close() call
int close(int sockfd) {
  char* fn_name = "close";
  get_next_fn((void**)&next_close,fn_name);

  /* fprintf(stderr, "close(%i)\n", sockfd); */
  /* next_close(ctrl_sock); */
  return next_close(sockfd);
}
