--- linux-3.16.3-clean/net/sched/sch_htb.c	2014-09-17 11:22:16.000000000 -0600
+++ ./linux-3.16.3-vtmininet/net/sched/sch_htb.c	2017-08-29 10:02:36.912974066 -0600
@@ -37,6 +37,9 @@
 #include <linux/rbtree.h>
 #include <linux/workqueue.h>
 #include <linux/slab.h>
+#include <linux/rcupdate.h> // for accessing parent process
+#include <linux/sched.h> // for accessing current
+
 #include <net/netlink.h>
 #include <net/sch_generic.h>
 #include <net/pkt_sched.h>
@@ -97,6 +100,7 @@ struct htb_prio {
 struct htb_class {
 	struct Qdisc_class_common common;
 	struct psched_ratecfg	rate;
+	int dilation;
 	struct psched_ratecfg	ceil;
 	s64			buffer, cbuffer;/* token bucket depth/rate */
 	s64			mbuffer;	/* max wait time */
@@ -857,6 +861,26 @@ next:
 
 	} while (cl != start);
 
+	/* polling host process about dilation info; update rate when necessary */
+	int dilation = 0;
+	if (current)
+	{
+		/* get tdf, possibly an updated one */
+		rcu_read_lock();
+		struct task_struct *parent = rcu_dereference(current->real_parent);
+		// printk("[info] [process %d] in htb_dequeue_tree: current's parent(%d) dilation(%d)\n", current->pid, parent->pid, parent->dilation);
+		dilation = parent->dilation;
+		rcu_read_unlock();
+	}
+	/* until now, still not sure if this the should-dilated htb class */
+	if (dilation > 0 && dilation != cl->dilation)
+	{
+		cl->dilation = dilation; // update dilation
+		printk("[info] [process %d] in htb_dequeue_tree: change tdf to %d\n", current->pid, cl->dilation);
+		psched_ratecfg_dilate(&cl->rate, cl->dilation); // update rate
+	}
+
+
 	if (likely(skb != NULL)) {
 		bstats_update(&cl->bstats, skb);
 		cl->un.leaf.deficit[level] -= qdisc_pkt_len(skb);
@@ -895,10 +919,13 @@ ok:
 
 	if (!sch->q.qlen)
 		goto fin;
+
 	q->now = ktime_to_ns(ktime_get());
+	// printk("[info] in htb_dequeue q->now in ns: %lld\n", q->now);
 	start_at = jiffies;
 
 	next_event = q->now + 5LLU * NSEC_PER_SEC;
+	// printk("[info] in htb_dequeue next_event in ns: %lld\n", next_event);
 
 	for (level = 0; level < TC_HTB_MAXDEPTH; level++) {
 		/* common case optimization - skip event handler quickly */
@@ -925,6 +952,8 @@ ok:
 				goto ok;
 		}
 	}
+	// printk("[info] in htb_dequeue next_event in ns: %lld\n", next_event);
+
 	sch->qstats.overlimits++;
 	if (likely(next_event > q->now)) {
 		if (!test_bit(__QDISC_STATE_DEACTIVATED,
@@ -1057,6 +1086,23 @@ static int htb_init(struct Qdisc *sch, s
 		q->rate2quantum = 1;
 	q->defcls = gopt->defcls;
 
+	/*
+	int copy = -1;
+	if (current)
+	{
+		if (current->dilation > 0)
+		{
+			copy = q->rate2quantum;
+			// wrong!!! modify rate2quantum will crash OS
+			// q->rate2quantum = q->rate2quantum / current->dilation;
+		}
+	} else {
+		printk("[panic] in htb_init current is NULL");
+	}
+
+	printk("[info] in htb_init: q->rate2quantum(%d), old copy(%d)", q->rate2quantum, copy);
+	*/
+	
 	return 0;
 }
 
@@ -1111,7 +1157,12 @@ static int htb_dump_class(struct Qdisc *
 
 	memset(&opt, 0, sizeof(opt));
 
+	// printk("[info] in htb_dump_class: cl->rate.rate_bytes_ps(%llu)\n", cl->rate.rate_bytes_ps);
+	// printk("[info] in htb_dump_class: opt.rate.rate(%u)\n", opt.rate.rate);
 	psched_ratecfg_getrate(&opt.rate, &cl->rate);
+	// printk("[info] in htb_dump_class: cl->rate.rate_bytes_ps(%llu)\n", cl->rate.rate_bytes_ps);
+	// printk("[info] in htb_dump_class: opt.rate.rate(%u)\n", opt.rate.rate);
+
 	opt.buffer = PSCHED_NS2TICKS(cl->buffer);
 	psched_ratecfg_getrate(&opt.ceil, &cl->ceil);
 	opt.cbuffer = PSCHED_NS2TICKS(cl->cbuffer);
@@ -1477,7 +1528,11 @@ static int htb_change_class(struct Qdisc
 
 	ceil64 = tb[TCA_HTB_CEIL64] ? nla_get_u64(tb[TCA_HTB_CEIL64]) : 0;
 
+	// printk("[info] in htb_change_class: rate64(%llu)\n", rate64);
+	// printk("[info] in htb_change_class: 1. write hopt->rate.rate(%u) to cl->rate.rate_bytes_ps\n", hopt->rate.rate);
 	psched_ratecfg_precompute(&cl->rate, &hopt->rate, rate64);
+	// printk("[info] in htb_change_class: 2. after dilation, cl->rate.rate_bytes_ps = %llu\n", cl->rate.rate_bytes_ps);
+
 	psched_ratecfg_precompute(&cl->ceil, &hopt->ceil, ceil64);
 
 	/* it used to be a nasty bug here, we have to check that node
