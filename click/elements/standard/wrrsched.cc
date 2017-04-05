// -*- c-basic-offset: 4 -*-
/*
 * wrrsched.{cc,hh} -- weighted round robin scheduler element
 * Robert Morris, Eddie Kohler
 *
 * Copyright (c) 1999-2000 Massachusetts Institute of Technology
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
#include <click/args.hh>
#include <click/error.hh>
#include "wrrsched.hh"
CLICK_DECLS

WRRSched::WRRSched()
    : _next(0), _tokens(0), _weights(0), _signals(0)
{
}

int
WRRSched::initialize(ErrorHandler *errh)
{
    if (!(_signals = new NotifierSignal[ninputs()]))
        return errh->error("out of memory!");
    for (int i = 0; i < ninputs(); i++)
        _signals[i] = Notifier::upstream_empty_signal(this, i);
    _tokens = _weights[0];
    return 0;
}

void
WRRSched::parse_weights(const String str)
{
    const char *cstr = str.c_str();
    unsigned int left = 0;
    unsigned int i = 0;
    unsigned int j;
    for (j = 0; j < strlen(cstr); j++) {
        if (cstr[j] == ',') {
            _weights[i] = atoi(cstr + left);
            left = j+1;
            i++;
        }
    }
    _weights[i] = atoi(cstr + left);
}

int
WRRSched::configure(Vector<String> &conf, ErrorHandler *errh)
{
    String str;
    unsigned int w = 1;
    unsigned int n = ninputs();
    if (!(_weights = new unsigned int[n]))
        return errh->error("out of memory!");
    if (Args(conf, this, errh).read_m("WEIGHTS", str).complete() < 0)
        return -1;

    parse_weights(str);

    return 0;
}

void
WRRSched::cleanup(CleanupStage)
{
    delete[] _signals;
    delete[] _weights;
}

Packet *
WRRSched::pull(int)
{
    unsigned int n = ninputs();
    for (unsigned int j = 0; j < n; j++) {
        Packet *p = (_signals[_next] ? input(_next).pull() : 0);

        _tokens--;
        // if (_tokens <= 0 || !p) {
        if (_tokens <= 0) {
            _next = (_next + 1) % n;
            _tokens = _weights[_next];
        }
            
        if (p)
            return p;
        else
            return 0;
    }
    return 0;
}

// int
// WRRSched::change_param(const String &s, Element *e, void *vparam,
//                        ErrorHandler *errh)
// {
//     switch ((intptr_t)vparam) {
//     case h_weights:
//         String str;
//         StringArg().parse(s, str);
//         parse_weights(str);
//         break;
//     }
//     return 0;
// }

// void
// WRRSched::add_handlers()
// {
//     add_write_handler("weights", change_param, h_weights, Handler::RAW);
// }

CLICK_ENDDECLS
EXPORT_ELEMENT(WRRSched)
