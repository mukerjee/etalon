--- linux-3.16.3-clean/net/sched/act_police.c	2014-09-17 11:22:16.000000000 -0600
+++ ./linux-3.16.3-vtmininet/net/sched/act_police.c	2017-08-29 10:02:36.912974066 -0600
@@ -201,6 +201,7 @@ override:
 	if (R_tab) {
 		police->rate_present = true;
 		psched_ratecfg_precompute(&police->rate, &R_tab->rate, 0);
+		printk("[info] in tcf_act_police_locate: police->rate(%llu), R_tab->rate(%llu), dilation(%d)\n", police->rate.rate_bytes_ps, R_tab->rate.rate, current->dilation);
 		qdisc_put_rtab(R_tab);
 	} else {
 		police->rate_present = false;
