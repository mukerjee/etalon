// -*- c-basic-offset: 4 -*-
#ifndef CLICK_SOLSTICE_HH
#define CLICK_SOLSTICE_HH
#include <click/sols.h>
#include <click/element.hh>
#include <click/timer.hh>
CLICK_DECLS

/*
=c

Solstice()

=s control

Computes OCS Solstice Schedules

=d

TODO

=h

*/

class Solstice : public Element {
  public:
    Solstice() CLICK_COLD;

    const char *class_name() const	{ return "Solstice"; }
    int configure(Vector<String> &, ErrorHandler *) CLICK_COLD;
    int initialize(ErrorHandler *) CLICK_COLD;

    void run_timer(Timer *);

  private:
    sols_t _s;
    int *_traffic_matrix;
    int *_enqueued_matrix;
    int *_dequeued_matrix;
    Timer _timer;
    int _num_hosts;
};

CLICK_ENDDECLS
#endif
