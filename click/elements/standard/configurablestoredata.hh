// -*- c-basic-offset: 4 -*-
#ifndef CLICK_CONFIGURABLESTOREDATA_HH
#define CLICK_CONFIGURABLESTOREDATA_HH
#include <click/element.hh>
CLICK_DECLS

/* =c
 * ConfigurableStoreData(OFFSET, DATA)
 * =s basicmod
 * changes packet data
 * =d
 *
 * Changes packet data starting at OFFSET to DATA.
 *
 * =a AlignmentInfo, click-align(1) */

class ConfigurableStoreData : public Element { public:

    ConfigurableStoreData() CLICK_COLD;

    const char *class_name() const		{ return "ConfigurableStoreData"; }
    const char *port_count() const		{ return PORTS_1_1; }
    void add_handlers() CLICK_COLD;
    
    int configure(Vector<String> &, ErrorHandler *) CLICK_COLD;
    bool can_live_reconfigure() const   { return true; }

    Packet *simple_action(Packet *);

  private:

    unsigned _offset;
    String _data;
};

CLICK_ENDDECLS
#endif
