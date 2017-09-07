/*
 * dontpullswitch.{cc,hh} -- 
 * 
 *
 * Copyright (c) 2009 Intel Corporation
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
#include "dontpullswitch.hh"
#include <click/confparse.hh>
#include <click/error.hh>
#include <click/args.hh>
CLICK_DECLS

DontPullSwitch::DontPullSwitch()
  : _next(0), _dont(-1), _signals(0)
{
}

int
DontPullSwitch::configure(Vector<String> &conf, ErrorHandler *errh)
{
    int dont = 0;
    if (Args(conf, this, errh).read_p("DONT", dont).complete() < 0)
	return -1;
    set_dont(dont);
    return 0;
}

int
DontPullSwitch::initialize(ErrorHandler *errh)
{
    if (!(_signals = new NotifierSignal[ninputs()]))
       return errh->error("out of memory!");
    for (int i = 0; i < ninputs(); i++)
	_signals[i] = Notifier::upstream_empty_signal(this, i);
    return 0;
}

void
DontPullSwitch::cleanup(CleanupStage)
{
    delete[] _signals;
}

Packet *
DontPullSwitch::pull(int)
{
    int n = ninputs();
    int i = _next;
    if (_next == _dont)
      i = (i + 1) % n;
    if (_next == _dont) // (e.g., _dont == 0 and n == 0)
      return 0;
    for (int j = 0; j < n; j++) {
	Packet *p = (_signals[i] ? input(i).pull() : 0);
	i++;
	if (i >= n)
	    i = 0;
	if (p) {
	    _next = i;
	    return p;
	}
    }
    return 0;
}

void
DontPullSwitch::set_dont(int dont)
{
    _dont = (dont < 0 || dont >= ninputs() ? -1 : dont);
}

int
DontPullSwitch::write_param(const String &s, Element *e, void *, ErrorHandler *errh)
{
    DontPullSwitch *sw = static_cast<DontPullSwitch *>(e);
    int input;
    if (!IntArg().parse(s, input))
	return errh->error("syntax error");
    sw->set_dont(input);
    return 0;
}

void
DontPullSwitch::add_handlers()
{
    add_data_handlers("switch", Handler::OP_READ, &_dont);
    add_write_handler("switch", write_param);
}


CLICK_ENDDECLS
EXPORT_ELEMENT(DontPullSwitch)

