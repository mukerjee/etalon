// -*- c-basic-offset: 4 -*-
/*
 * solstice.{cc,hh} -- Computes OCS Solstice Schedules
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
#include "solstice.hh"
#include <click/confparse.hh>
#include <click/error.hh>
#include <sys/select.h>
#if defined(__APPLE__) && defined(__MACH__)
# include <CoreServices/CoreServices.h>
# include <mach/mach_time.h>
# ifndef __osx__
#  define __osx__
# endif
#endif

CLICK_DECLS

Solstice::Solstice() : _timer(this)
{
}

int
Solstice::configure(Vector<String> &conf, ErrorHandler *errh)
{
    int num_hosts, reconfig_delay, tdf;
    uint32_t circuit_bw, packet_bw;
    if (Args(conf, this, errh)
        .read_mp("NUM_HOSTS", num_hosts)
	.read_mp("CIRCUIT_BW", BandwidthArg(), circuit_bw)
	.read_mp("PACKET_BW", BandwidthArg(), packet_bw)
	.read_mp("RECONFIG_DELAY", reconfig_delay)
	.read_mp("TDF", tdf)
        .complete() < 0)
        return -1;
    _num_hosts = num_hosts;
    
    if (_num_hosts == 0)
        return -1;
    _traffic_matrix = (long long *)malloc(sizeof(long long) * _num_hosts * _num_hosts);
    _enqueued_matrix = (long long *)malloc(sizeof(long long) * _num_hosts * _num_hosts);
    _dequeued_matrix = (long long *)malloc(sizeof(long long) * _num_hosts * _num_hosts);

    sols_init(&_s, _num_hosts);
    _s.night_len = reconfig_delay * tdf;  // reconfiguration us
    _s.week_len = 2000 * tdf;  // schedule max length us
    _s.min_day_len = 9 * reconfig_delay * tdf;  // minimum configuration length us
    // _s.skip_trim = true;
    _s.day_len_align = 1;  // ???
    _s.link_bw = int(circuit_bw / 1000000); // 9Gbps (in bytes / us)
    _s.pack_bw = int(packet_bw / 1000000); // 1Gbps (in bytes / us)

    _print = 0;

    return 0;
}
 
int
Solstice::initialize(ErrorHandler *)
{
    _timer.initialize(this);
    
#if defined(__linux__)
    sched_setscheduler(getpid(), SCHED_RR, NULL);
#endif

    _timer.schedule_now();
    return 0;
}

void
Solstice::run_timer(Timer *)
{
#if defined(__osx__)
    static mach_timebase_info_data_t sTimebaseInfo;
    mach_timebase_info(&sTimebaseInfo);
    uint64_t start_time = mach_absolute_time();
#elif defined(__linux__)
    struct timespec start_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time);
#else
    errh->error("only implemented for linux and osx");
    return -1;
#endif

    int duration = int(_s.week_len * 0.9);
    while(1) {
        // wait for a while. solstice is actually much quicker to run
	// than the week length. (e.g., 30us to run without prints)
        long long elapsed_nano = 0;

#if defined(__osx__)
        uint64_t ts_new;
        while (elapsed_nano < duration) {
            ts_new = mach_absolute_time();
            elapsed_nano = (ts_new - start_time) *
                sTimebaseInfo.numer / sTimebaseInfo.denom;
        }
        start_time = ts_new;
#elif defined(__linux__)
        struct timespec ts_new;
        while (elapsed_nano < duration) {
            clock_gettime(CLOCK_MONOTONIC, &ts_new);
            elapsed_nano = (1000000000 * ts_new.tv_sec + ts_new.tv_nsec)
                - (1000000000 * start_time.tv_sec + start_time.tv_nsec);
        }
        start_time = ts_new;
#endif

	// gather traffic matrix from queues
        for (int src = 0; src < _num_hosts; src++) {
            for (int dst = 0; dst < _num_hosts; dst++) {
		int i = src * _num_hosts + dst;
		char handler[500];
		sprintf(handler, "hybrid_switch/q%d%d/q.enqueue_bytes", src, dst);
                _enqueued_matrix[i] = atoll(HandlerCall::call_read(handler,
								   this).c_str());
		sprintf(handler, "hybrid_switch/q%d%d/q.dequeue_bytes", src, dst);
                _dequeued_matrix[i] = atoll(HandlerCall::call_read(handler,
								   this).c_str());
		_traffic_matrix[i] = _enqueued_matrix[i] - _dequeued_matrix[i];

		sprintf(handler, "hybrid_switch/q%d%d/q.bytes", src, dst);
		_traffic_matrix[i] = atoll(HandlerCall::call_read(handler,
								  this).c_str());

		// sprintf(handler, "hybrid_switch/q%d%d/lq.bytes", src, dst);
		// _traffic_matrix[i] += atoll(HandlerCall::call_read(handler,
		// 						   this).c_str());

		// sprintf(handler, "hybrid_switch/ps/q%d%d.bytes", src, dst);
		// _traffic_matrix[i] += atoll(HandlerCall::call_read(handler,
		// 						   this).c_str());

		if (_traffic_matrix[i] < 0)
		    _traffic_matrix[i] = 0;
		if (src == 0 && dst == 1 && _print == 0) {
		    sprintf(handler, "hybrid_switch/q%d%d/q.length", src, dst);
		    int len = atoi(HandlerCall::call_read(handler, 
		    					  this).c_str());
		    sprintf(handler, "hybrid_switch/q%d%d/lq.length", src, dst);
		    int loss_len = atoi(HandlerCall::call_read(handler, 
							       this).c_str());
		    sprintf(handler, "hybrid_switch/ps/q%d%d.length", src, dst);
		    int pslen = atoi(HandlerCall::call_read(handler, 
							    this).c_str());
		    printf("e = %lld, d = %lld, tm = %lld, len = %d, losslen = %d, pslen= %d\n",
		    	   _enqueued_matrix[i], _dequeued_matrix[i], 
                           _traffic_matrix[i], len, loss_len, pslen);

		    sprintf(handler, "hybrid_switch/q%d%d/q.length", 2, 3);
		    len = atoi(HandlerCall::call_read(handler, 
						      this).c_str());
		    sprintf(handler, "hybrid_switch/q%d%d/lq.length", 2, 3);
		    loss_len = atoi(HandlerCall::call_read(handler, 
							   this).c_str());
		    sprintf(handler, "hybrid_switch/ps/q%d%d.length", 2, 3);
		    pslen = atoi(HandlerCall::call_read(handler, 
							this).c_str());
		    printf("e = %lld, d = %lld, tm = %lld, len = %d, losslen = %d, pslen= %d\n",
		    	   _enqueued_matrix[i], _dequeued_matrix[i], 
			   _traffic_matrix[i], len, loss_len, pslen);
		}
            }
        }
	_print = (_print + 1) % 10000;

        uint64_t cap = _s.week_len * (_s.link_bw + _s.pack_bw);

        /* setup the demand here */
        for (int src = 0; src < _num_hosts; src++) {
            for (int dst = 0; dst < _num_hosts; dst++) {
                uint64_t v = _traffic_matrix[src * _num_hosts + dst];
		if (v > cap)
		    v = cap;
                if (v == 0) continue;
                sols_mat_set(&_s.future, src, dst, v);
            }
        }

	if(_print == 0) {
	    printf("[demand]\n");
	    for (int src = 0; src < _num_hosts; src++) {
		for (int dst = 0; dst < _num_hosts; dst++) {
		    if (dst > 0) printf(" ");
		    uint64_t v = _traffic_matrix[src * _num_hosts + dst]; // _s.demand.m
		    if (v == 0)
			printf(".");
		    else
			printf("%ld", v);
		}
		printf("\n");
	    }
	}

        sols_schedule(&_s);
        sols_check(&_s);


	// each configuration string should be at most _num_hosts*3 length
	// each duration string should be at most 4 length
	// there are _s.nday days and nights
	// add an additional factor of 2 for number of days and delimiters
	char *schedule = (char *)malloc(sizeof(char) * _s.nday * 2 * (4 + _num_hosts*3) * 2);
	sprintf(schedule, "%d ", _s.nday * 2);
        
        for (int i = 0; i < _s.nday; i++) {
	    if (i > 0)
		sprintf(&(schedule[strlen(schedule)]), " ");
            sols_day_t *day = &_s.sched[i];
	    sprintf(&(schedule[strlen(schedule)]), "%ld ", day->len);

	    for (int dst = 0; dst < _num_hosts; dst++) {
                int src = day->input_ports[dst];
                if (src < 0) {
                    printf("SOLSTICE BAD PORT\n");
                    return;
                }
		sprintf(&(schedule[strlen(schedule)]), "%d/", src);
            }
	    schedule[strlen(schedule)-1] = '\0';

	    // nights
	    sprintf(&(schedule[strlen(schedule)]), " %ld ", _s.night_len);
	    for (int src = 0; src < _num_hosts; src++) {
	    	sprintf(&(schedule[strlen(schedule)]), "%d/", -1);
	    }
	    schedule[strlen(schedule)-1] = '\0';
        }

	if (_print == 0) {
            // clock_gettime(CLOCK_MONOTONIC, &ts_new);
            // int elapsed_nano = (1000000000 * ts_new.tv_sec + ts_new.tv_nsec)
            //     - (1000000000 * start_time.tv_sec + start_time.tv_nsec);
	    // printf("elapsed = %f us\n", elapsed_nano / 1000.0);
	    printf("schedule == %s\n", schedule);
	}

        // tell schedule runner
        HandlerCall::call_write("runner.setSchedule", schedule, this);

	free(schedule);
    }
}

CLICK_ENDDECLS
EXPORT_ELEMENT(Solstice)
ELEMENT_REQUIRES(userlevel)
