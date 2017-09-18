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
        .read_mp("NUM_HOSTS", _num_hosts)
	.read_mp("SOURCE", source)
        .complete() < 0)
        return -1;
    
    if (_num_hosts == 0)
        return -1;
    _traffic_matrix = (long long *)malloc(sizeof(long long) * _num_hosts * _num_hosts);
    _adu_enqueue_matrix = (long long *)malloc(sizeof(long long) * _num_hosts * _num_hosts);
    _enqueue_matrix = (long long *)malloc(sizeof(long long) * _num_hosts * _num_hosts);
    _adu_dequeue_matrix = (long long *)malloc(sizeof(long long) * _num_hosts * _num_hosts);
    _dequeue_matrix = (long long *)malloc(sizeof(long long) * _num_hosts * _num_hosts);

    memset(_traffic_matrix, 0, sizeof(long long) * _num_hosts * _num_hosts);
    memset(_adu_enqueue_matrix, 0, sizeof(long long) * _num_hosts * _num_hosts);
    memset(_enqueue_matrix, 0, sizeof(long long) * _num_hosts * _num_hosts);
    memset(_adu_dequeue_matrix, 0, sizeof(long long) * _num_hosts * _num_hosts);
    memset(_dequeue_matrix, 0, sizeof(long long) * _num_hosts * _num_hosts);

    _print = 0;

    return 0;
}
 
int
EstimateTraffic::initialize(ErrorHandler *errh)
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

    _queue_dequeue_bytes = (HandlerCall **)malloc(sizeof(HandlerCall *) * _num_hosts * _num_hosts);
    for(int src = 0; src < _num_hosts; src++) {
        for(int dst = 0; dst < _num_hosts; dst++) {
            char handler[500];
            sprintf(handler, "hybrid_switch/q%d%d/q.dequeue_bytes", src, dst);
            _queue_dequeue_bytes[src * _num_hosts + dst] = new HandlerCall(handler);
            _queue_dequeue_bytes[src * _num_hosts + dst]->initialize(HandlerCall::f_read, this, errh);
        }
    }

    _queue_dequeue_bytes_no_headers = (HandlerCall **)malloc(sizeof(HandlerCall *) * _num_hosts * _num_hosts);
    for(int src = 0; src < _num_hosts; src++) {
        for(int dst = 0; dst < _num_hosts; dst++) {
            char handler[500];
            sprintf(handler, "hybrid_switch/q%d%d/q.dequeue_bytes_no_headers", src, dst);
            _queue_dequeue_bytes_no_headers[src * _num_hosts + dst] = new HandlerCall(handler);
            _queue_dequeue_bytes_no_headers[src * _num_hosts + dst]->initialize(HandlerCall::f_read, this, errh);
        }
    }

    _queue_enqueue_bytes = (HandlerCall **)malloc(sizeof(HandlerCall *) * _num_hosts * _num_hosts);
    for(int src = 0; src < _num_hosts; src++) {
        for(int dst = 0; dst < _num_hosts; dst++) {
            char handler[500];
            sprintf(handler, "hybrid_switch/q%d%d/q.enqueue_bytes", src, dst);
            _queue_enqueue_bytes[src * _num_hosts + dst] = new HandlerCall(handler);
            _queue_enqueue_bytes[src * _num_hosts + dst]->initialize(HandlerCall::f_read, this, errh);
        }
    }

    _queue_bytes = (HandlerCall **)malloc(sizeof(HandlerCall *) * _num_hosts * _num_hosts);
    for(int src = 0; src < _num_hosts; src++) {
        for(int dst = 0; dst < _num_hosts; dst++) {
            char handler[500];
            sprintf(handler, "hybrid_switch/q%d%d/q.bytes", src, dst);
            _queue_bytes[src * _num_hosts + dst] = new HandlerCall(handler);
            _queue_bytes[src * _num_hosts + dst]->initialize(HandlerCall::f_read, this, errh);
        }
    }

    _timer.schedule_now();
    return 0;
}

void
EstimateTraffic::run_timer(Timer *)
{
    while(1) {
	// gather traffic matrix from queues
	int clientSocket;
	int i;
	int nbytes;
	struct traffic_info info;
	struct sockaddr_storage *clientAddr;
	socklen_t sinSize = sizeof(struct sockaddr_storage);
	fd_set read_fd_set;
	struct timeval timeout;
	timeout.tv_sec = 0;
	timeout.tv_usec = 0;

	read_fd_set = _active_fd_set;
	int rc = select(FD_SETSIZE, &read_fd_set, NULL, NULL, &timeout);
	if (rc < 0) {
	    perror("select");
	    exit(EXIT_FAILURE);
	}

	if (rc > 0) {
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
			// fprintf(stderr, "[CTRL] SRC: %s DST: %s SIZE: %ld\n", info.src,
			// 	    info.dst, info.size);

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
			int src = atoi(&info.src[pos]) - 1;
			if (strlen(&info.src[pos]) == 1) // physical hosts
			    src--;

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
			int dst = atoi(&info.dst[pos]) -1;
			if (strlen(&info.dst[pos]) == 1) // physical hosts
			    dst--;

			_adu_enqueue_matrix[src * _num_hosts + dst] += info.size;
		    }
		}
	    }
	}

	for (int src = 0; src < _num_hosts; src++) { // update dequeue bytes and tm
	    for (int dst = 0; dst < _num_hosts; dst++) {
		int i = src * _num_hosts + dst;
		_enqueue_matrix[i] = atoll(_queue_enqueue_bytes[i]->call_read().c_str());
		_dequeue_matrix[i] = atoll(_queue_dequeue_bytes[i]->call_read().c_str());
		_adu_dequeue_matrix[i] = atoll(_queue_dequeue_bytes_no_headers[i]->call_read().c_str());
		_traffic_matrix[i] = _adu_enqueue_matrix[i] - _adu_dequeue_matrix[i];
		if (_traffic_matrix[i] < 0)
		    _traffic_matrix[i] = 0;
	    }
	}

	if (source != "ADU") {
	    for (int src = 0; src < _num_hosts; src++) {
		for (int dst = 0; dst < _num_hosts; dst++) {
		    int i = src * _num_hosts + dst;
		    _traffic_matrix[i] = atoll(_queue_bytes[i]->call_read().c_str());
		    if (_traffic_matrix[i] < 0)
			_traffic_matrix[i] = 0;
		}
	    }
        }

	// copy TM to store for handler
	pthread_mutex_lock(&lock);
	output_traffic_matrix = String();
	for(int src = 0; src < _num_hosts; src++) {
	    for(int dst = 0; dst < _num_hosts; dst++) {
		if (output_traffic_matrix != "")
		    output_traffic_matrix += " ";
		output_traffic_matrix += String(_traffic_matrix[src * _num_hosts + dst]);
	    }
	}
	pthread_mutex_unlock(&lock);

	_print = (_print + 1) % 100000;
	
	if (_print == 0) {
	    // printf("tm on et side = %s\n", output_traffic_matrix.c_str());
	    int psrc = 0;
	    int pdst = 1;
	    int i = psrc * _num_hosts + pdst;
	    char handler[500];
	    printf("\n");
	    sprintf(handler, "hybrid_switch/q%d%d/q.length", psrc, pdst);
	    int len = atoi(HandlerCall::call_read(handler,
						  this).c_str());
	    sprintf(handler, "hybrid_switch/ps/q%d%d.length", psrc, pdst);
	    int pslen = atoi(HandlerCall::call_read(handler,
						    this).c_str());
	    printf("%s: (0, 1)\tae = %lld, ad = %lld, e = %lld, d = %lld, tm = %lld, len = %d, pslen= %d\n",
		   source.c_str(), _adu_enqueue_matrix[i], _adu_dequeue_matrix[i],
		   _enqueue_matrix[i], _dequeue_matrix[i],
		   _traffic_matrix[i], len, pslen);

	    psrc = 3;
	    pdst = 1;
	    i = psrc * _num_hosts + pdst;
	    sprintf(handler, "hybrid_switch/q%d%d/q.length", psrc, pdst);
	    len = atoi(HandlerCall::call_read(handler,
					      this).c_str());
	    sprintf(handler, "hybrid_switch/ps/q%d%d.length", psrc, pdst);
	    pslen = atoi(HandlerCall::call_read(handler,
						this).c_str());
	    printf("%s: (2, 3)\tae = %lld, ad = %lld, e = %lld, d = %lld, tm = %lld, len = %d, pslen= %d\n",
		   source.c_str(), _adu_enqueue_matrix[i], _adu_dequeue_matrix[i],
		   _enqueue_matrix[i], _dequeue_matrix[i],
		   _traffic_matrix[i], len, pslen);
	}
    }
}

String
EstimateTraffic::get_traffic(Element *e, void *)
{
    EstimateTraffic *et = static_cast<EstimateTraffic *>(e);
    pthread_mutex_lock(&et->lock);
    String out = String(et->output_traffic_matrix);
    pthread_mutex_unlock(&et->lock);
    return out;
}

int
EstimateTraffic::set_source(const String &str, Element *e, void *, ErrorHandler *)
{
    EstimateTraffic *et = static_cast<EstimateTraffic *>(e);
    et->source = String(str);
    return 0;
}

void
EstimateTraffic::add_handlers()
{
    add_write_handler("setSource", set_source, 0);
    add_read_handler("getTraffic", get_traffic, 0);
}

CLICK_ENDDECLS
EXPORT_ELEMENT(EstimateTraffic)
ELEMENT_REQUIRES(userlevel)
