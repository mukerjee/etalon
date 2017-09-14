// -*- c-basic-offset: 4 -*-
#ifndef CLICK_ESTIMATE_TRAFFIC_HH
#define CLICK_ESTIMATE_TRAFFIC_HH
#include <click/element.hh>
#include <click/timer.hh>
#include <pthread.h>
CLICK_DECLS

/*
=c

EstimateTraffic()

=s control

Estimates OCS Traffic

=d

TODO

=h

*/

#define ADU_PORT "8123"

class EstimateTraffic : public Element {
  public:
    EstimateTraffic() CLICK_COLD;

    const char *class_name() const	{ return "EstimateTraffic"; }
    int configure(Vector<String> &, ErrorHandler *) CLICK_COLD;
    int initialize(ErrorHandler *) CLICK_COLD;
    void add_handlers() CLICK_COLD;

    void run_timer(Timer *);
    String source;
    String output_traffic_matrix;
    pthread_mutex_t lock;

  private:
    static int set_source(const String&, Element*, void*, ErrorHandler*) CLICK_COLD;
    static String get_traffic(Element *e, void *user_data);

    int _serverSocket;
    fd_set _active_fd_set;

    int _num_hosts;

    long long *_enqueue_matrix;
    long long *_dequeue_matrix;
    long long *_traffic_matrix;
    Timer _timer;
    int _print;

    HandlerCall **_queue_dequeue_bytes;
    HandlerCall **_queue_dequeue_bytes_no_headers;
    HandlerCall **_queue_enqueue_bytes;
    HandlerCall **_queue_bytes;
};

CLICK_ENDDECLS
#endif
