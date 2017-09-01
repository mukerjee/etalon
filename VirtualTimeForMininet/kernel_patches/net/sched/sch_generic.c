--- linux-3.16.3-clean/net/sched/sch_generic.c	2014-09-17 11:22:16.000000000 -0600
+++ ./linux-3.16.3-vtmininet/net/sched/sch_generic.c	2017-08-29 10:02:36.908974111 -0600
@@ -917,7 +917,14 @@ void psched_ratecfg_precompute(struct ps
 {
 	memset(r, 0, sizeof(*r));
 	r->overhead = conf->overhead;
+	r->undilated_rate_bytes_ps = max_t(u64, conf->rate, rate64);
 	r->rate_bytes_ps = max_t(u64, conf->rate, rate64);
+	if (current && current->dilation > 1)
+	{
+		r->rate_bytes_ps = r->undilated_rate_bytes_ps / current->dilation;
+		printk("[info] in psched_ratecfg_precompute: rate(%llu) dilated(%d) from origin(%llu)\n", r->rate_bytes_ps, current->dilation, r->undilated_rate_bytes_ps);
+	}
+
 	r->linklayer = (conf->linklayer & TC_LINKLAYER_MASK);
 	r->mult = 1;
 	/*
@@ -944,5 +951,32 @@ void psched_ratecfg_precompute(struct ps
 			r->shift++;
 		}
 	}
+	// printk("[info] ratecfg = {rate(%llu), mult(%d), overhead(%d), linklayer(%d), shift(%d)}\n", r->rate_bytes_ps, r->mult, r->overhead, r->linklayer, r->shift);
 }
 EXPORT_SYMBOL(psched_ratecfg_precompute);
+
+void psched_ratecfg_dilate(struct psched_ratecfg *r, int dilation)
+{
+	if (r && dilation > 1)
+	{
+		r->rate_bytes_ps = r->undilated_rate_bytes_ps / dilation;
+		printk("[info] [process %d] in psched_ratecfg_dilate: change rate to %llu due to change of tdf(%d)\n", current->pid, r->rate_bytes_ps, dilation);
+
+		r->mult = 1;
+		r->shift = 0;
+		if (r->rate_bytes_ps > 0)
+		{
+			/* not sure what this code does, but better redo it after updating rate_bytes_ps */
+			u64 factor = NSEC_PER_SEC;
+			for (;;) {
+				r->mult = div64_u64(factor, r->rate_bytes_ps);
+				if (r->mult & (1U << 31) || factor & (1ULL << 63))
+					break;
+				factor <<= 1;
+				r->shift++;
+			}
+		}
+	}
+}
+EXPORT_SYMBOL(psched_ratecfg_dilate);
+
