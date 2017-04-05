#ifndef CHECKPATTERN_HH
#define CHECKPATTERN_HH
#include <click/element.hh>
CLICK_DECLS

/*
 * CheckPattern(len)
 *
 * Check the packets generated by SendPattern(n)
 */

class CheckPattern : public Element {

  unsigned _len;

 public:

  CheckPattern() CLICK_COLD;
  ~CheckPattern() CLICK_COLD;

  const char *class_name() const	{ return "CheckPattern"; }
  const char *port_count() const	{ return PORTS_1_1; }

  int configure(Vector<String> &, ErrorHandler *) CLICK_COLD;

  Packet *simple_action(Packet *);

};

CLICK_ENDDECLS
#endif
