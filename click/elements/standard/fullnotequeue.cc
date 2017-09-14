// -*- c-basic-offset: 4 -*-
/*
 * fullnotequeue.{cc,hh} -- queue element that notifies on full
 * Eddie Kohler
 *
 * Copyright (c) 2004-2007 Regents of the University of California
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
#include "fullnotequeue.hh"
#include <pthread.h>
#include <clicknet/tcp.h>

CLICK_DECLS

FullNoteQueue::FullNoteQueue()
{
    pthread_mutex_init(&_lock, NULL);
    enqueue_bytes = 0;
    dequeue_bytes = 0;
}

void *
FullNoteQueue::cast(const char *n)
{
    if (strcmp(n, "FullNoteQueue") == 0)
	return (FullNoteQueue *)this;
    else if (strcmp(n, Notifier::FULL_NOTIFIER) == 0)
	return static_cast<Notifier*>(&_full_note);
    else
	return NotifierQueue::cast(n);
}

int
FullNoteQueue::configure(Vector<String> &conf, ErrorHandler *errh)
{
    _full_note.initialize(Notifier::FULL_NOTIFIER, router());
    _full_note.set_active(true, false);
    return NotifierQueue::configure(conf, errh);
}

int
FullNoteQueue::live_reconfigure(Vector<String> &conf, ErrorHandler *errh)
{
    int r = NotifierQueue::live_reconfigure(conf, errh);
    if (r >= 0 && size() < capacity() && _q)
	_full_note.wake();
    return r;
}

void
FullNoteQueue::push(int, Packet *p)
{
    pthread_mutex_lock(&_lock);
    // Code taken from SimpleQueue::push().
    Storage::index_type h = head(), t = tail(), nt = next_i(t);

    if (nt != h) {
        push_success(h, t, nt, p);
        enqueue_bytes += p->length();
    }
    else {
	push_failure(p);
    }
    pthread_mutex_unlock(&_lock);
}

Packet *
FullNoteQueue::pull(int)
{
    pthread_mutex_lock(&_lock);
    // Code taken from SimpleQueue::deq.
    Storage::index_type h = head(), t = tail(), nh = next_i(h);

    if (h != t) {
        Packet *p = pull_success(h, nh);
        dequeue_bytes += p->length();

	if (p->has_transport_header()) {
	    int tplen = p->transport_length();
	    const click_ip *ipp = p->ip_header();
	    if (ipp->ip_p == IP_PROTO_TCP) { // TCP
		tplen -= p->tcp_header()->th_off * 4;
	    }
	    else if (ipp->ip_p == IP_PROTO_UDP) { // UDP
		tplen -= 8;
	    }
	    dequeue_bytes_no_headers += tplen;
	}
	pthread_mutex_unlock(&_lock);
        return p;
    }
    else {
	pthread_mutex_unlock(&_lock);
	return pull_failure();
    }
}

#if CLICK_DEBUG_SCHEDULING
String
FullNoteQueue::read_handler(Element *e, void *)
{
    FullNoteQueue *fq = static_cast<FullNoteQueue *>(e);
    return "nonempty " + fq->_empty_note.unparse(fq->router())
	+ "\nnonfull " + fq->_full_note.unparse(fq->router());
}

void
FullNoteQueue::add_handlers()
{
    NotifierQueue::add_handlers();
    add_read_handler("notifier_state", read_handler, 0);
}
#else
String
FullNoteQueue::read_enqueue_bytes(Element *e, void *)
{
    FullNoteQueue *fq = static_cast<FullNoteQueue *>(e);
    return String(fq->enqueue_bytes);
}

String
FullNoteQueue::read_dequeue_bytes(Element *e, void *)
{
    FullNoteQueue *fq = static_cast<FullNoteQueue *>(e);
    return String(fq->dequeue_bytes);
}

String
FullNoteQueue::read_dequeue_bytes_no_headers(Element *e, void *)
{
    FullNoteQueue *fq = static_cast<FullNoteQueue *>(e);
    return String(fq->dequeue_bytes_no_headers);
}

String
FullNoteQueue::read_bytes(Element *e, void *)
{
    FullNoteQueue *fq = static_cast<FullNoteQueue *>(e);

    pthread_mutex_lock(&(fq->_lock));
    int byte_count = 0;
    Storage::index_type h = fq->head(), t = fq->tail();
    while (h != t) {
	byte_count += fq->_q[h]->length();
	h = fq->next_i(h);
    }
    String r = String(byte_count);
    pthread_mutex_unlock(&(fq->_lock));
    
    return r;
}

void
FullNoteQueue::add_handlers()
{
    NotifierQueue::add_handlers();
    add_read_handler("enqueue_bytes", read_enqueue_bytes, 0);
    add_read_handler("dequeue_bytes", read_dequeue_bytes, 0);
    add_read_handler("dequeue_bytes_no_headers", read_dequeue_bytes_no_headers, 0);
    add_read_handler("bytes", read_bytes, 0);
}
#endif

CLICK_ENDDECLS
ELEMENT_REQUIRES(NotifierQueue)
EXPORT_ELEMENT(FullNoteQueue FullNoteQueue-FullNoteQueue)
