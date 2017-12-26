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

#define LOCAL_CTRL_DEVNAME "enp8s0d1"
#define SWITCH_CTRL_IP "10.2.10.9"
#define SWITCH_CTRL_PORT "8123"

#define MAX_FD 4096

struct traffic_info {
  char src[INET_ADDRSTRLEN];
  char dst[INET_ADDRSTRLEN];
  size_t size;
};

struct traffic_info* info_table[MAX_FD] = {NULL};
char local_ip[NI_MAXHOST];
int ctrl_sock = -1;

static int (*next_socket)(int domain, int type, int protocol) = NULL;
static int (*next_send)(int socket, const void *buffer, size_t length,
                        int flags) = NULL;
static int (*next_write)(int fd, void *buffer, size_t size) = NULL;
static int (*next_close)(int sockfd) = NULL;

void get_local_ip(int fd, char* saddr) {
  struct sockaddr_in localAddress;
  socklen_t addressLength = sizeof(localAddress);
  getsockname(fd, (struct sockaddr*)&localAddress, &addressLength);
  strncpy(saddr, inet_ntoa(localAddress.sin_addr),
          INET_ADDRSTRLEN);
}

void get_remote_ip(int fd, char* raddr) {
  socklen_t len;
  struct sockaddr_storage addr;

  len = sizeof addr;
  getpeername(fd, (struct sockaddr*)&addr, &len);

  if (addr.ss_family == AF_INET) {
    struct sockaddr_in *s = (struct sockaddr_in *)&addr;
    inet_ntop(AF_INET, &s->sin_addr, raddr, INET_ADDRSTRLEN);
  }
}

static void open_ctrl_socket() {
  if(ctrl_sock == -1){
    struct addrinfo hints, *res, *p;

    char *host=SWITCH_CTRL_IP, *port=SWITCH_CTRL_PORT;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM; //TCP

    if (getaddrinfo(host, port, &hints, &res) != 0) {
      perror("getaddrinfo() failed");
      exit(-1);
    }

    for(p = res; p != NULL; p = p->ai_next)  {
      if ((ctrl_sock = next_socket(p->ai_family, p->ai_socktype,
                                   p->ai_protocol)) == -1) {
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
  get_next_fn((void**)&next_socket, fn_name);
  get_next_fn((void**)&next_close, fn_name);

  open_ctrl_socket();

  int sockfd = next_socket(domain, type, protocol);

  if (sockfd > MAX_FD) {
    fprintf(stderr, "Sockfd larger than MAX_FD\n");
    return -1;
  }
  if (sockfd < 0) {
    return sockfd;
  }

  info_table[sockfd] = (struct traffic_info *)malloc(sizeof(struct traffic_info));
  memset(info_table[sockfd], 0, sizeof(struct traffic_info));

  return sockfd;
}

// Wrap up the write() call
ssize_t write(int fd, void *buffer, size_t size) {
  ssize_t nbytes = -1;
  char* fn_name = "write";
  get_next_fn((void**)&next_write, fn_name);

  ssize_t true_size = next_write(fd, buffer, size);

  // Get the destination address
  if (info_table[fd] && fd != ctrl_sock) {
    get_local_ip(fd, info_table[fd]->src);
    get_remote_ip(fd, info_table[fd]->dst);
    /* fprintf(stderr, "LOCAL_IP: %s REMOTE_IP: %s\n", info_table[fd]->src, */
    /* 	    info_table[fd]->dst); */
    info_table[fd]->size = true_size;
    nbytes = next_write(ctrl_sock, info_table[fd], sizeof(struct traffic_info));

    if (nbytes != sizeof(struct traffic_info)){
      fprintf(stderr, "Failed to send ctrl message\n");
      return nbytes;
    }
  /* fprintf(stderr, "SIZE: %ld\n", true_size); */
  }
  return true_size;
}

// Wrap up the send() call
ssize_t send(int fd, const void *buffer, size_t size, int flags) {
  ssize_t nbytes = -1;
  char* fn_name = "send";
  get_next_fn((void**)&next_send, fn_name);

  ssize_t true_size = next_send(fd, buffer, size, flags);

  // Get the destination address
  if (info_table[fd] && fd != ctrl_sock) {
    get_local_ip(fd, info_table[fd]->src);
    get_remote_ip(fd, info_table[fd]->dst);
    /* fprintf(stderr, "LOCAL_IP: %s REMOTE_IP: %s\n", info_table[fd]->src, */
    /* 	    info_table[fd]->dst); */
    info_table[fd]->size = true_size;
    nbytes = next_send(ctrl_sock, info_table[fd], sizeof(struct traffic_info), flags);
    if (nbytes != sizeof(struct traffic_info)){
      fprintf(stderr, "Failed to send ctrl message\n");
      return nbytes;
    }
  }
  return true_size;
}

// Wrap up the close() call
int close(int sockfd) {
  char* fn_name = "close";
  get_next_fn((void**)&next_close, fn_name);

  free(info_table[sockfd]);
  info_table[sockfd] = NULL;

  /* fprintf(stderr, "close(%i)\n", sockfd); */
  /* next_close(ctrl_sock); */
  return next_close(sockfd);
}
