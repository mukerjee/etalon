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

CLICK_DECLS

Solstice::Solstice() : _timer(this)
{
}

int
Solstice::configure(Vector<String> &conf, ErrorHandler *errh)
{
    String num_hosts;
    if (Args(conf, this, errh)
        .read_mp("NUM_HOSTS", num_hosts)
        .complete() < 0)
        return -1;
    _num_hosts = atoi(num_hosts.c_str());
    if (_num_hosts == 0)
        return -1;
    _traffic_matrix = (int **)malloc(sizeof(int*) * _num_hosts);
    for (int i = 0; i < _num_hosts; i++) {
        _traffic_matrix[i] = (int *)malloc(sizeof(int) * _num_hosts);
    }
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
        // gather traffic matrix from queues
        char queue[500];
        for (int src = 0; src < _num_hosts; src++) {
            for (int dst = 0; dst < _num_hosts; dst++) {
                sprintf(queue, "hybrid_switch/q%d%d.length", src, dst);
                _traffic_matrix[src][dst] = atoi(HandlerCall::call_read(queue, this).c_str());
            }
        }

        // do solstice here

        schedule = "6 180 1/2/3/0 20 4/4/4/4 180 2/3/0/1 20 4/4/4/4 180 3/0/1/2 20 4/4/4/4";

        // tell schedule runner
        HandlerCall::call_write("runner.setSchedule", schedule, this);

        // remove this...
        _timer.schedule_after(Timestamp(1));
        return;
    }
}

CLICK_ENDDECLS
EXPORT_ELEMENT(Solstice)
ELEMENT_REQUIRES(userlevel)
