// -*- c-basic-offset: 4 -*-
/*
 * hybridswitch.{cc,hh} -- weighted round robin scheduler element
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
#include "hybridswitch.hh"
CLICK_DECLS

HybridSwitch::HybridSwitch()
    : _next(0), _tokens(0), _weights(0), _signals(0)
{
}

int
HybridSwitch::initialize(ErrorHandler *errh)
{
    unsigned int n = noutputs();
    if (!(_signals = new NotifierSignal*[n]))
        return errh->error("out of memory!");
    for (unsigned int i = 0; i < n; i++) {
        if (!(_signals[i] = new NotifierSignal[n]))
            return errh->error("out of memory!");
    }
    for (unsigned int i = 0; i < n; i++) {
        for (unsigned int j = 0; j < n; j++) {
            _signals[i][j] = Notifier::upstream_empty_signal(this, i*n + j);
        }
    }

    if (!(_tokens = new unsigned int[n]))
        return errh->error("out of memory!");
    for (unsigned int i = 0; i < n; i++) {
        _tokens[i] = _weights[i][0];
    }

    if (!(_next = new unsigned int[n]))
        return errh->error("out of memory!");
    for (unsigned int i = 0; i < n; i++) {
        _next[i] = 0;
    }
    return 0;
}

void
HybridSwitch::parse_weights(const String str)
{
    const char *cstr = str.c_str();
    unsigned int left = 0;
    unsigned int i = 0;
    unsigned int j = 0;
    for (unsigned int k = 0; k < strlen(cstr); k++) {
        if (cstr[k] == ',' || cstr[k] == ';') {
            _weights[i][j] = atoi(cstr + left);
            left = k+1;
            if (cstr[k] == ';') {
                j++;
                i = 0;
            } else {
                i++;
            }
        }
    }
}

int
HybridSwitch::configure(Vector<String> &conf, ErrorHandler *errh)
{
    String str;
    unsigned int n = noutputs();
    if (!(_weights = new unsigned int*[n]))
        return errh->error("out of memory!");
    for (unsigned int i = 0; i < n; i++) {
        if (!(_weights[i] = new unsigned int[n]))
            return errh->error("out of memory!");
    }
    if (Args(conf, this, errh).read_m("WEIGHTS", str).complete() < 0)
        return -1;

    parse_weights(str);

    return 0;
}

void
HybridSwitch::cleanup(CleanupStage)
{
    unsigned int n = noutputs();
    for (unsigned int i = 0; i < n; i++) {
        delete[] _signals[i];
        delete[] _weights[i];
    }
    delete[] _signals;
    delete[] _weights;
    delete[] _next;
    delete[] _tokens;
}

Packet *
HybridSwitch::pull(int port)
{
    unsigned int n = noutputs();
    for (unsigned int j = 0; j < n; j++) {
        Packet *p = (_signals[port][_next[port]] ?
                     input(n*port + _next[port]).pull() : 0);

        _tokens[port]--;
        if (_tokens[port] <= 0 || !p) {
            _next[port] = (_next[port] + 1) % n;
            _tokens[port] = _weights[port][_next[port]];
        }

        if (p)
            return p;
    }
    return 0;
}

// int
// HybridSwitch::change_param(const String &s, Element *e, void *vparam,
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
// HybridSwitch::add_handlers()
// {
//     add_write_handler("weights", change_param, h_weights, Handler::RAW);
// }

CLICK_ENDDECLS
EXPORT_ELEMENT(HybridSwitch)
