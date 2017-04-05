// -*- c-basic-offset: 4 -*-
#ifndef CLICK_HYBRIDSWITCH_HH
#define CLICK_HYBRIDSWITCH_HH
#include <click/element.hh>
#include <click/notifier.hh>
CLICK_DECLS

/*
 * =c
 * HybridSwitch
 * =s scheduling
 * pulls from round-robin inputs, with weights
 * =io
 * one output, zero or more inputs
 * =d
 * Each time a pull comes in the output, pulls from its inputs
 * in turn until one produces a packet. When the next pull
 * comes in, it starts from the input after the one that
 * last produced a packet. This amounts to a round robin
 * scheduler.
 *
 * The inputs usually come from Queues or other pull schedulers.
 * WeightedRoundRobinSched uses notification to avoid pulling from empty inputs.
 *
 * =a PrioSched, StrideSched, DRRSched, RoundRobinSwitch, SimpleRoundRobinSched
 */

class HybridSwitch : public Element { public:

    HybridSwitch() CLICK_COLD;

    const char *class_name() const	{ return "HybridSwitch"; }
    const char *port_count() const	{ return "-/-"; }
    const char *processing() const	{ return PULL; }
    const char *flags() const		{ return "S0"; }

    int configure(Vector<String>&, ErrorHandler*) CLICK_COLD;
    int initialize(ErrorHandler *) CLICK_COLD;
    void cleanup(CleanupStage) CLICK_COLD;
    // void add_handlers() CLICK_COLD;

    Packet *pull(int port);

  private:

    unsigned int *_next;
    unsigned int *_tokens;
    unsigned int **_weights;
    NotifierSignal **_signals;

    enum { h_weights };
    // static String read_param(Element *, void *) CLICK_COLD;
    // static int change_param(const String &, Element *, void *, ErrorHandler *);
    void parse_weights(const String str);

};

CLICK_ENDDECLS
#endif
