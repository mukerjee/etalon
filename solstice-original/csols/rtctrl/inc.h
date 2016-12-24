#include <errno.h>
#include "snf/snf.h"
#include "sols/sols.h"
#include "swctrl/swctrl.hh"

#include <stdint.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cassert>
#include <ctime>

#include <fcntl.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <netinet/tcp.h>
#include <netinet/ether.h>
#include <sys/socket.h>
#include <sched.h>
#include <signal.h>

#include <vector>
using std::vector;

// #define NHOST 8
#define NLANE 64
#define NQUEUE 8

#include "cpuaff.h"
#include "mac.h"
#include "hprint.h"
#include "mprint.h"
#include "sol8.h"
#include "snf.h"
#include "packer.h"
#include "udpsender.h"
#include "nicsync.h"
#include "demrecv.h"
#include "ctrlgen.h"
#include "flags.h"
#include "main.h"
