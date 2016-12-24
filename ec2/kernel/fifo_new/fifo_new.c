/*
 * net/sched/sch_fifo_new.c	The simplest FIFO_NEW queue.
 *
 *		This program is free software; you can redistribute it and/or
 *		modify it under the terms of the GNU General Public License
 *		as published by the Free Software Foundation; either version
 *		2 of the License, or (at your option) any later version.
 *
 * Authors:	Alexey Kuznetsov, <kuznet@ms2.inr.ac.ru>
 */

#include <linux/module.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/skbuff.h>
#include <net/pkt_sched.h>

/* 1 band FIFO_NEW pseudo-"scheduler" */

static int fifo_new_enqueue(struct sk_buff *skb, struct Qdisc *sch)
{
	if (likely(skb_queue_len(&sch->q) < sch->limit))
		return qdisc_enqueue_tail(skb, sch);

	return qdisc_reshape_fail(skb, sch);
}

static int fifo_new_init(struct Qdisc *sch, struct nlattr *opt)
{
	bool bypass;

    printk(KERN_INFO "making new fifo queue\n");

	if (opt == NULL) {
		u32 limit = qdisc_dev(sch)->tx_queue_len;

		sch->limit = limit;
	} else {
		struct tc_fifo_qopt *ctl = nla_data(opt);

		if (nla_len(opt) < sizeof(*ctl))
			return -EINVAL;

		sch->limit = ctl->limit;
	}

    bypass = sch->limit >= 1;

	if (bypass)
		sch->flags |= TCQ_F_CAN_BYPASS;
	else
		sch->flags &= ~TCQ_F_CAN_BYPASS;
	return 0;
}

static int fifo_new_dump(struct Qdisc *sch, struct sk_buff *skb)
{
	struct tc_fifo_qopt opt = { .limit = sch->limit };

	if (nla_put(skb, TCA_OPTIONS, sizeof(opt), &opt))
		goto nla_put_failure;
	return skb->len;

 nla_put_failure:
	return -1;
}

struct Qdisc_ops fifo_new_qdisc_ops __read_mostly = {
	.id		=	"fifo_new",
	.priv_size	=	0,
	.enqueue	=	fifo_new_enqueue,
	.dequeue	=	qdisc_dequeue_head,
	.peek		=	qdisc_peek_head,
	.drop		=	qdisc_queue_drop,
	.init		=	fifo_new_init,
	.reset		=	qdisc_reset_queue,
	.change		=	fifo_new_init,
	.dump		=	fifo_new_dump,
	.owner		=	THIS_MODULE,
};

static int __init fifo_new_module_init(void)
{
    return register_qdisc(&fifo_new_qdisc_ops);
}

static void __exit fifo_new_module_exit(void)
{
    unregister_qdisc(&fifo_new_qdisc_ops);
}

module_init(fifo_new_module_init)
module_exit(fifo_new_module_exit)
MODULE_LICENSE("GPL");
