// -*- c-basic-offset: 4 -*-
/*
 * run_schedule.{cc,hh} -- Runs an optical switching schedule
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
#include "run_schedule.hh"
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

RunSchedule::RunSchedule() : _timer(this), _num_hosts(0)
{
}

int
RunSchedule::configure(Vector<String> &conf, ErrorHandler *errh)
{
    String num_hosts;
    if (Args(conf, this, errh)
        .read_mp("NUM_HOSTS", num_hosts)
        .complete() < 0)
        return -1;
    _num_hosts = atoi(num_hosts.c_str());
    if (_num_hosts == 0)
        return -1;
    next_schedule = "";
    return 0;
}
 
int
RunSchedule::initialize(ErrorHandler *errh)
{
    _timer.initialize(this);
    
#if defined(__linux__)
    sched_setscheduler(getpid(), SCHED_RR, NULL);
#endif

    _timer.schedule_now();
    return 0;
}

int
RunSchedule::handler(int, String &str, Element *t, const Handler *,
                     ErrorHandler *errh)
{
    ((RunSchedule *)t)->next_schedule = str;
    return 0;
}

Vector<String>
RunSchedule::split(const String &s, char delim) {
    Vector<String> elems;
    int prev = 0;
    for(int i = 0; i < s.length(); i++) {
        if (s[i] == delim) {
            elems.push_back(s.substring(prev, i-prev));
            prev = i+1;
        }
    }
    elems.push_back(s.substring(prev, s.length()-prev));
    return elems;
}

int
RunSchedule::execute_schedule(ErrorHandler *errh)
{
    // parse schedule...

    // '2 180 1/2/3/0 20 4/4/4/4'
    String current_schedule = next_schedule;
    if (current_schedule == "")
        return 0;
    Vector<String> v = RunSchedule::split(current_schedule, ' ');
    int num_configurations = atoi(v[0].c_str());

    int *durations = (int *)malloc(sizeof(int) * num_configurations);
    Vector<int> *configurations = new Vector<int>[num_configurations];
    int j = 0;
    Vector<String> c;
    for(int i = 1; i < v.size(); i+=2) {
        durations[j] = atoi(v[i].c_str());
        c = RunSchedule::split(v[i+1], '/');
        for(int k = 0; k < c.size(); k++) {
            configurations[j].push_back(atoi(c[k].c_str()));
        }
        j++;
    }
    

    // for each configuration in schedule
    for(int i = 0; i < num_configurations; i++) {
        Vector<int> configuration = configurations[i];
        int duration = durations[i]; // microseconds
#if defined(__osx__)
        static mach_timebase_info_data_t sTimebaseInfo;
        mach_timebase_info(&sTimebaseInfo);
        uint64_t start_time = mach_absolute_time();
#elif defined(__linux__)
        struct timespec start_time;
        clock_gettime(CLOCK_MONOTONIC, &start_time);
#endif

        // set configuration    
        for(int i = 0; i < _num_hosts; i++) {
            int src = i;
            int dst = configuration[i];
            char handler[500];
            char value[5];
            sprintf(handler, "hybrid_switch/circuit_link%d/ps.switch", src);
            sprintf(value, "%d", dst);
            HandlerCall::call_write(handler, value, this);

            // probably just remove this? we aren't signaling TCP to dump.
            sprintf(handler, "hybrid_switch/ecnr%d/s.switch", dst);
            sprintf(value, "%d", src);
            HandlerCall::call_write(handler, value, this);
        }

        // wait duration
        uint64_t elapsed_nano = 0;

#if defined(__osx__)
        uint64_t ts_new;

        while (elapsed_nano < duration * 1000) {
            ts_new = mach_absolute_time();
            elapsed_nano = (ts_new - start_time) *
                sTimebaseInfo.numer / sTimebaseInfo.denom;
        }
        start_time = ts_new;
#elif defined(__linux__)
        struct timespec ts_new;
                    
        while (elapsed_nano < duration * 1000) {
            clock_gettime(CLOCK_MONOTONIC, &ts_new);
            elapsed_nano = (1000000000 * ts_new.tv_sec + ts_new.tv_nsec)
                - (1000000000 * start_time.tv_sec + start_time.tv_nsec);
        }    
        start_time = ts_new;
#else
        errh->error("only implemented for linux and osx");
        return -1;
#endif
    }
    return 0;
}

void
RunSchedule::run_timer(Timer *)
{
    while(1) {
        int rc = execute_schedule(ErrorHandler::default_handler());
        if (rc)
            return;
    }
}

void
RunSchedule::add_handlers()
{
    set_handler("setSchedule", Handler::h_write | Handler::h_write_private,
                handler, 0, 0);
}

CLICK_ENDDECLS
EXPORT_ELEMENT(RunSchedule)
ELEMENT_REQUIRES(userlevel)
