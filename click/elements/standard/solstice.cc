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
#include <algorithm>  // max

CLICK_DECLS

Solstice::Solstice() : _timer(this)
{
}

int
Solstice::configure(Vector<String> &conf, ErrorHandler *errh)
{
    String num_hosts, circuit_bw;
    if (Args(conf, this, errh)
        .read_mp("NUM_HOSTS", num_hosts)
        .complete() < 0)
        return -1;
    _num_hosts = atoi(num_hosts.c_str());
    if (_num_hosts == 0)
        return -1;
    _traffic_matrix = (int *)malloc(sizeof(int) * _num_hosts * _num_hosts);
    _enqueued_matrix = (int *)malloc(sizeof(int) * _num_hosts * _num_hosts);
    _dequeued_matrix = (int *)malloc(sizeof(int) * _num_hosts * _num_hosts);
    return 0;
}
 
int
Solstice::initialize(ErrorHandler *errh)
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
    String schedule;
    
    while(1) {
	sols_init(&_s, _num_hosts);
	_s.night_len = 20;  // reconfiguration us
	_s.week_len = 3000;  // schedule max length us
	_s.avg_day_len = 40;  // ???
	_s.min_day_len = 40;  // minimum configuration length us
	_s.day_len_align = 1;  // ???
	_s.link_bw = 1125; // 9Gbps (in bytes / us)
	_s.pack_bw = 125; // 1Gbps (in bytes / us)

	// gather traffic matrix from queues
        char queue[500];
        for (int src = 0; src < _num_hosts; src++) {
            for (int dst = 0; dst < _num_hosts; dst++) {
		int i = src * _num_hosts + dst;
                sprintf(queue, "hybrid_switch/q%d%d.enqueue_bytes", src, dst);
                _enqueued_matrix[i] =
                    atoi(HandlerCall::call_read(queue, this).c_str());
                sprintf(queue, "hybrid_switch/q%d%d.dequeue_bytes", src, dst);
                _dequeued_matrix[i] =
                    atoi(HandlerCall::call_read(queue, this).c_str());
		_traffic_matrix[i] = std::max(_enqueued_matrix[i] -
					      _dequeued_matrix[i], 0);
            }
        }

        // do solstice here

        // int i, j;
        // uint64_t dat[NHOST * NHOST];

        // uint64_t cap = (s.week_len * s.link_bw / _num_hosts);

        // for (i = 0; i < NHOST; i++) {
        //     for (j = 0; j < NHOST; j++) {
        //         if (i == j) {
        //             continue;
        //         }

        //         dat[i * NHOST + j] = rand() % cap;
        //     }
        // }

        /* setup the demand here */
        for (int src = 0; src < _num_hosts; src++) {
            for (int dst = 0; dst < _num_hosts; dst++) {
                uint64_t v = _traffic_matrix[src * _num_hosts + dst];
                if (v == 0) continue;
                sols_mat_set(&_s.future, src, dst, v);
            }
        }

        sols_schedule(&_s);
        sols_check(&_s);

        // printf("[demand]\n");
        // for (int src = 0; src < _num_hosts; src++) {
        //     for (int dst = 0; dst < _num_hosts; dst++) {
        //         if (dst > 0) printf(" ");
        //         uint64_t v = _s.demand.m[src * _num_hosts + dst];
        //         if (v == 0)
        //             printf(".");
        //         else
        //             printf("%ld", v);
        //     }
        //     printf("\n");            
        // }
        
        char configuration[500];
        
        schedule = String();
        sprintf(configuration, "%d ", _s.nday);
        schedule.append(configuration);
        

        int *port_mapping = (int *)malloc(sizeof(int) * _num_hosts);
        for (int i = 0; i < _s.nday; i++) {
            sols_day_t *day = &_s.sched[i];
            sprintf(configuration, "%ld ", day->len);
            schedule.append(configuration);

            
            // printf("day #%d: T=" FMT_U64 "\n", i, day->len);
            for (int dst = 0; dst < _num_hosts; dst++) {
                int src = day->input_ports[dst];
                if (src < 0) {
                    printf("SOLSTICE BAD PORT\n");
                    return;
                }
                port_mapping[src] = dst;
                // if (day->is_dummy[dst]) {
                //     printf("  (%d -> %d)\n", src, dst);
                // } else {
                //     printf("  %d -> %d\n", src, dst);
                // }
            }

            configuration[0] = '\0';
            for (int src = 0; src < _num_hosts; src++) {
                char *p = &(configuration[strlen(configuration)]);
                sprintf(p, "%d/", port_mapping[src]);
            }
            configuration[strlen(configuration)-1] = '\0';
            schedule.append(configuration);
        }

        sols_cleanup(&_s);
        // printf("%s\n", schedule.c_str());

        // schedule = "6 180 1/2/3/0 20 4/4/4/4 180 2/3/0/1 20 4/4/4/4 180 3/0/1/2 20 4/4/4/4";

        // tell schedule runner
        HandlerCall::call_write("runner.setSchedule", schedule, this);

        // // remove this...
        // _timer.schedule_after(Timestamp(1));
        // return;
    }
}

CLICK_ENDDECLS
EXPORT_ELEMENT(Solstice)
ELEMENT_REQUIRES(userlevel)
