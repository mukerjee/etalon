// -*- c-basic-offset: 4 -*-
/*
 * estimate_traffic.{cc,hh} -- Estimates OCS traffic
 * Matt Mukerjee
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, subject to the conditions
 * listed in the Click LICENSE file. These conditions include: you must
 * preserve this copyright notice, and you cannot mention the copyright
 * holders in advertising related to the Software without their permission.
 * The Software is provided WITHOUT ANY WARRANTY, EXPRESS OR IMPLIED. This
 * notice is a summary of the Click LICENSE file; the license in that file is
 * legally binding.
 */

#include <click/config.h>
#include <click/handlercall.hh>
#include <click/args.hh>
#include "estimate_traffic.hh"
#include <click/confparse.hh>
#include <click/error.hh>
#include <sys/select.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <pthread.h>
#include <signal.h>
#include <errno.h>

CLICK_DECLS

struct traffic_info {
    char src[INET_ADDRSTRLEN];
    char dst[INET_ADDRSTRLEN];
    size_t size;
};

EstimateTraffic::EstimateTraffic() : _timer(this)
{
}

int
EstimateTraffic::configure(Vector<String> &conf, ErrorHandler *errh)
{
    if (Args(conf, this, errh)
        .read_mp("NUM_HOSTS", num_hosts)
	.read_mp("SOURCE", source)
        .complete() < 0)
        return -1;
    
    if (num_hosts == 0)
        return -1;
    traffic_matrix = (long long *)malloc(sizeof(long long) * num_hosts * num_hosts);
    _enqueued_matrix = (long long *)malloc(sizeof(long long) * num_hosts * num_hosts);
    _dequeued_matrix = (long long *)malloc(sizeof(long long) * num_hosts * num_hosts);

    memset(traffic_matrix, 0, sizeof(long long) * num_hosts * num_hosts);
    memset(_enqueued_matrix, 0, sizeof(long long) * num_hosts * num_hosts);
    memset(_dequeued_matrix, 0, sizeof(long long) * num_hosts * num_hosts);    

    _print = 0;

    return 0;
}
 
int
EstimateTraffic::initialize(ErrorHandler *)
{
    _timer.initialize(this);
    pthread_mutex_init(&lock, NULL);
    
#if defined(__linux__)
    sched_setscheduler(getpid(), SCHED_RR, NULL);
#endif


    struct addrinfo hints, *res, *p;
    int yes = 1;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if (getaddrinfo(NULL, ADU_PORT, &hints, &res) != 0) {
        perror("getaddrinfo() failed");
        pthread_exit(NULL);
    }

    for(p = res; p != NULL; p = p->ai_next) {
        if ((_serverSocket = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("Could not open socket");
            continue;
        }

        if (setsockopt(_serverSocket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
            perror("Socket setsockopt() failed");
            close(_serverSocket);
            continue;
        }

        if (bind(_serverSocket, p->ai_addr, p->ai_addrlen) == -1) {
            perror("Socket bind() failed");
            close(_serverSocket);
            continue;
        }

        if (listen(_serverSocket, 5) == -1) {
            perror("Socket listen() failed");
            close(_serverSocket);
            continue;
        }
        break;
    }

    freeaddrinfo(res);

    if (p == NULL) {
        fprintf(stderr, "Could not find a socket to bind to.\n");
        pthread_exit(NULL);
    }

    FD_ZERO(&_active_fd_set);
    FD_SET(_serverSocket, &_active_fd_set);


    _timer.schedule_now();
    return 0;
}

void
EstimateTraffic::run_timer(Timer *)
{
    while(1) {

	// gather traffic matrix from queues
	pthread_mutex_lock(&lock);
	if (source == "ADU") {
	    int clientSocket;
	    int i;
	    int nbytes;
	    struct traffic_info info;
	    struct sockaddr_storage *clientAddr;
	    socklen_t sinSize = sizeof(struct sockaddr_storage);
	    fd_set read_fd_set;

	    read_fd_set = _active_fd_set;
	    if (select(FD_SETSIZE, &read_fd_set, NULL, NULL, NULL) < 0) {
		perror("select");
		exit(EXIT_FAILURE);
	    }

	    for (i = 0; i < FD_SETSIZE; ++i) {
		if (FD_ISSET(i, &read_fd_set)) {
		    if (i == _serverSocket) {
			clientAddr = (struct sockaddr_storage *)malloc(sinSize);
			if ((clientSocket = accept(_serverSocket, 
						   (struct sockaddr *)clientAddr, 
						   &sinSize)) == -1) {
			    free(clientAddr);
			    perror("Could not accept() connection");
			    exit (EXIT_FAILURE);
			}
			FD_SET(clientSocket, &_active_fd_set);
			fprintf(stderr, "New connection: %d\n", clientSocket);
		    }
		    else {
			nbytes = read(i, &info, sizeof(info));
			if (nbytes == 0) {
			    fprintf(stderr, "Closing socket\n");
			    close(i);
			    FD_CLR(i, &_active_fd_set);
			    break;
			}
			if (nbytes < 0) {
			    perror("Socket read() failed");
			    close(i);
			    FD_CLR (i, &_active_fd_set);
			    exit (EXIT_FAILURE);
			}
			fprintf(stderr, "[CTRL] SRC: %s DST: %s SIZE: %ld\n", info.src, 
				info.dst, info.size);
		    }
		}
	    }

	    int dot_count = 0;
	    int pos;
	    for(pos = 0; pos < INET_ADDRSTRLEN; pos++) {
		if (info.src[pos] == '.') {
		    dot_count++;
		    if (dot_count == 3) {
			pos++;
			break;
		    }
		}
	    }
	    int src = atoi(&info.src[pos]);

	    dot_count = 0;
	    for(pos = 0; pos < INET_ADDRSTRLEN; pos++) {
		if (info.dst[pos] == '.') {
		    dot_count++;
		    if (dot_count == 3) {
			pos++;
			break;
		    }
		}
	    }
	    int dst = atoi(&info.dst[pos]);

	    int j = src * num_hosts + dst;
	    _enqueued_matrix[j] += info.size;

	    char handler[500];
	    sprintf(handler, "hybrid_switch/q%d%d/q.dequeue_bytes", src, dst);
	    _dequeued_matrix[j] = atoll(HandlerCall::call_read(handler,
							       this).c_str());
	    traffic_matrix[j] = _enqueued_matrix[j] - _dequeued_matrix[j];
	    if (traffic_matrix[j] < 0)
		traffic_matrix[j] = 0;
	} else { // not ADU
	    for (int src = 0; src < num_hosts; src++) {
		for (int dst = 0; dst < num_hosts; dst++) {
		    int i = src * num_hosts + dst;
		    char handler[500];

		    sprintf(handler, "hybrid_switch/q%d%d/q.enqueue_bytes", src, dst);
		    _enqueued_matrix[i] = atoll(HandlerCall::call_read(handler,
								       this).c_str());
		    sprintf(handler, "hybrid_switch/q%d%d/q.dequeue_bytes", src, dst);
		    _dequeued_matrix[i] = atoll(HandlerCall::call_read(handler,
								       this).c_str());
		    sprintf(handler, "hybrid_switch/q%d%d/q.bytes", src, dst);
		    traffic_matrix[i] = atoll(HandlerCall::call_read(handler,
								     this).c_str());
		    if (traffic_matrix[i] < 0)
			traffic_matrix[i] = 0;
		}
	    }
        }
	pthread_mutex_unlock(&lock);

	_print = (_print + 1) % 10000;
	
	// int psrc = 0;
	// int pdst = 1;
	// if (psrc == 0 && pdst == 1 && _print == 0) {
	//     int i = psrc * num_hosts + pdst;
	//     char handler[500];
	//     printf("\n");
	//     sprintf(handler, "hybrid_switch/q%d%d/q.length", psrc, pdst);
	//     int len = atoi(HandlerCall::call_read(handler, 
	// 					  this).c_str());
	//     sprintf(handler, "hybrid_switch/q%d%d/lq.length", psrc, pdst);
	//     int loss_len = atoi(HandlerCall::call_read(handler, 
	// 					       this).c_str());
	//     sprintf(handler, "hybrid_switch/ps/q%d%d.length", psrc, pdst);
	//     int pslen = atoi(HandlerCall::call_read(handler, 
	// 					    this).c_str());
	//     printf("e = %lld, d = %lld, tm = %lld, len = %d, losslen = %d, pslen= %d\n",
	// 	   _enqueued_matrix[i], _dequeued_matrix[i], 
	// 	   _traffic_matrix[i], len, loss_len, pslen);

	//     sprintf(handler, "hybrid_switch/q%d%d/q.length", 2, 3);
	//     len = atoi(HandlerCall::call_read(handler, 
	// 				      this).c_str());
	//     sprintf(handler, "hybrid_switch/q%d%d/lq.length", 2, 3);
	//     loss_len = atoi(HandlerCall::call_read(handler, 
	// 					   this).c_str());
	//     sprintf(handler, "hybrid_switch/ps/q%d%d.length", 2, 3);
	//     pslen = atoi(HandlerCall::call_read(handler, 
	// 					this).c_str());
	//     printf("e = %lld, d = %lld, tm = %lld, len = %d, losslen = %d, pslen= %d\n",
	// 	   _enqueued_matrix[i], _dequeued_matrix[i], 
	// 	   _traffic_matrix[i], len, loss_len, pslen);
	// }
    }
}

String
EstimateTraffic::get_traffic(Element *e, void *)
{
    EstimateTraffic *et = static_cast<EstimateTraffic *>(e);
    pthread_mutex_lock(&et->lock);
    String out;
    for(int src = 0; src < et->num_hosts; src++) {
	for(int dst = 0; dst < et->num_hosts; dst++) {
	    if (out != "")
		out += " ";
	    out += et->traffic_matrix[src *et->num_hosts + dst];
	}
    }
    pthread_mutex_unlock(&et->lock);
    return out;
}

int
EstimateTraffic::handler(int, String &str, Element *t, const Handler *,
                     ErrorHandler *)
{
    EstimateTraffic *et = static_cast<EstimateTraffic *>(t);
    et->source = String(str);
    return 0;
}

void
EstimateTraffic::add_handlers()
{
    set_handler("setSource", Handler::h_write | Handler::h_write_private,
                handler, 0, 0);
    add_read_handler("getTraffic", get_traffic, 0);
}

CLICK_ENDDECLS
EXPORT_ELEMENT(EstimateTraffic)
ELEMENT_REQUIRES(userlevel)
