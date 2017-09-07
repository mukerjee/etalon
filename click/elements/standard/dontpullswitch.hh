#ifndef CLICK_DONTPULLSWITCH_HH
#define CLICK_DONTPULLSWITCH_HH
#include <click/notifier.hh>
CLICK_DECLS

/*
=c

DontPullSwitch([INPUT])

=s scheduling

Round robins from everything except the set input

=d

=h switch read/write

Return or set the K parameter.

=a SimplePullSwitch, StaticPullSwitch, PrioSched, RoundRobinSched,
StrideSched, Switch */

class DontPullSwitch : public Element { public:

    DontPullSwitch() CLICK_COLD;

    const char *class_name() const		{ return "DontPullSwitch"; }
    const char *port_count() const	{ return "-/1"; }
    const char *processing() const	{ return PULL; }
    const char *flags() const		{ return "S0"; }

    int configure(Vector<String> &, ErrorHandler *) CLICK_COLD;
    int initialize(ErrorHandler *errh) CLICK_COLD;
    void cleanup(CleanupStage stage) CLICK_COLD;
    void add_handlers() CLICK_COLD;

    void set_dont(int dont);

    Packet *pull(int port);

  protected:
    int _next;
    int _dont;
    NotifierSignal *_signals;
    static int write_param(const String &, Element *, void *, 
                           ErrorHandler *) CLICK_COLD;
};

CLICK_ENDDECLS
#endif
