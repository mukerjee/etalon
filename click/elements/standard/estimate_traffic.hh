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

#define ADU_PORT "8888"

class EstimateTraffic : public Element {
  public:
    EstimateTraffic() CLICK_COLD;

    const char *class_name() const	{ return "EstimateTraffic"; }
    int configure(Vector<String> &, ErrorHandler *) CLICK_COLD;
    int initialize(ErrorHandler *) CLICK_COLD;
    void add_handlers() CLICK_COLD;

    void run_timer(Timer *);
    String source;
    long long *traffic_matrix;
    int num_hosts;
    pthread_mutex_t lock;

  private:
    static int handler(int, String&, Element*, const Handler*, ErrorHandler*);
    static String get_traffic(Element *e, void *user_data);

    int _serverSocket;
    fd_set _active_fd_set;

    long long *_enqueued_matrix;
    long long *_dequeued_matrix;
    Timer _timer;
    int _print;
};

CLICK_ENDDECLS
#endif
