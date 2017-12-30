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
#include <sys/stat.h>
#include <unistd.h>

#define LOCAL_CTRL_DEVNAME "enp8s0d1"
#define SWITCH_CTRL_IP "10.2.10.9"
#define SWITCH_CTRL_PORT "8123"

#define EXIT_FAILED -1

struct traffic_info {
  char src[INET_ADDRSTRLEN];
  char dst[INET_ADDRSTRLEN];
  size_t size;
};

char local_ip[NI_MAXHOST];
int ctrl_sock = -1;

// Find the library version of the function that we are wrapping
static void get_next_fn(void** next_fn, char* fn) {
  char* msg;
  *next_fn = dlsym(RTLD_NEXT, fn);
  if ((msg = dlerror()) != NULL) {
    fprintf(stderr, "dlopen failed on %s: %s\n", fn, msg);
    exit(EXIT_FAILED);
  }
}

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
      int (*next_socket)(int, int, int);
      get_next_fn((void**)&next_socket, "socket");
      if ((ctrl_sock = next_socket(p->ai_family, p->ai_socktype,
                                   p->ai_protocol)) == -1) {
        perror("Could not open socket");
        continue;
      }

      if (connect(ctrl_sock, p->ai_addr, p->ai_addrlen) == -1)  {
	int (*next_close)(int);
	get_next_fn((void**)&next_close, "close");
        next_close(ctrl_sock);
        perror("Could not connect to socket");
        continue;
      }

      break;
    }
    freeaddrinfo(res);
  }
}

void send_adu_info(int fd, int true_size) {
  struct stat statbuf;
  fstat(fd, &statbuf);
  if (!S_ISSOCK(statbuf.st_mode))
    return;

  if (fd == ctrl_sock)
    return;

  open_ctrl_socket();

  struct traffic_info adu_info;
  get_local_ip(fd, adu_info.src);
  get_remote_ip(fd, adu_info.dst);
  adu_info.size = true_size;
  /* fprintf(stderr, "LOCAL_IP: %s REMOTE_IP: %s\n", adu_info.src, adu_info.dst); */

  ssize_t (*next_write)(int, const void*, size_t);
  get_next_fn((void**)&next_write, "write");
  ssize_t nbytes = next_write(ctrl_sock, &adu_info, sizeof(struct traffic_info));
  if (nbytes != sizeof(struct traffic_info)){
    fprintf(stderr, "Failed to send ctrl message\n");
    exit(EXIT_FAILED);
  }
}

// Wrap up the write() call
ssize_t write(int fd, const void *buffer, size_t size) {
  ssize_t (*next_write)(int, const void*, size_t);
  get_next_fn((void**)&next_write, "write");

  ssize_t true_size = next_write(fd, buffer, size);

  send_adu_info(fd, true_size);

  return true_size;
}

// Wrap up the send() call
ssize_t send(int fd, const void *buffer, size_t size, int flags) {
  ssize_t (*next_send)(int, const void*, size_t, int);
  get_next_fn((void**)&next_send, "send");

  ssize_t true_size = next_send(fd, buffer, size, flags);

  send_adu_info(fd, true_size);

  return true_size;
}
