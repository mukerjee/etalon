--- linux-3.16.3-clean/include/net/sch_generic.h	2014-09-17 11:22:16.000000000 -0600
+++ ./linux-3.16.3-vtmininet/include/net/sch_generic.h	2017-08-29 10:02:36.904974155 -0600
@@ -684,6 +684,7 @@ static inline struct sk_buff *skb_act_cl
 
 struct psched_ratecfg {
 	u64	rate_bytes_ps; /* bytes per second */
+	u64 undilated_rate_bytes_ps; /* undilated Bps, should never overwrite after init in htb_change_class() */
 	u32	mult;
 	u16	overhead;
 	u8	linklayer;
@@ -705,6 +706,8 @@ void psched_ratecfg_precompute(struct ps
 			       const struct tc_ratespec *conf,
 			       u64 rate64);
 
+void psched_ratecfg_dilate(struct psched_ratecfg *r, int dilation);
+
 static inline void psched_ratecfg_getrate(struct tc_ratespec *res,
 					  const struct psched_ratecfg *r)
 {
@@ -716,6 +719,19 @@ static inline void psched_ratecfg_getrat
 	 */
 	res->rate = min_t(u64, r->rate_bytes_ps, ~0U);
 
+	/*
+	if (current)
+	{
+		if (current->dilation > 0)
+		{
+			// r->rate_bytes_ps = r->rate_bytes_ps / current->dilation * 1000;
+		}
+		printk("[info] in psched_ratecfg_getrate res->rate(%llu), r->rate_bytes_ps(%llu)\n", res->rate, r->rate_bytes_ps, current->dilation);
+	} else {
+		printk("[info] in psched_ratecfg_getrate current is NULL");
+	}
+	*/
+
 	res->overhead = r->overhead;
 	res->linklayer = (r->linklayer & TC_LINKLAYER_MASK);
 }
