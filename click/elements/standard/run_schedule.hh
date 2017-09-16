// -*- c-basic-offset: 4 -*-
#ifndef CLICK_RUNSCHEDULE_HH
#define CLICK_RUNSCHEDULE_HH
#include <click/element.hh>
#include <click/timer.hh>
#include <pthread.h>
CLICK_DECLS

/*
=c

RunSchedule()

=s control

Runs an optical circuit switching schedule

=d

TODO

=h block write-only

Write this handler to block execution for a specified number of seconds.

*/

class RunSchedule : public Element {
  public:
    RunSchedule() CLICK_COLD;

    const char *class_name() const	{ return "RunSchedule"; }
    int configure(Vector<String> &, ErrorHandler *) CLICK_COLD;
    int initialize(ErrorHandler *) CLICK_COLD;
    void add_handlers() CLICK_COLD;

    void run_timer(Timer *);

    String next_schedule;
    bool do_resize;
    pthread_mutex_t lock;

  private:

    static int handler(const String&, Element*, void*, ErrorHandler*);
    static int resize_handler(const String&, Element*, void*, ErrorHandler*);
    static Vector<String> split(const String&, char);
    int execute_schedule(ErrorHandler *);

    Timer _timer;
    int _num_hosts;
    int _big_buffer_size;
    int _small_buffer_size;
    HandlerCall **_queue_capacity;
    HandlerCall **_pull_switch;
    int _print;
};

CLICK_ENDDECLS
#endif
