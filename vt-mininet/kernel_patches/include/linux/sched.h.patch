--- linux-3.16.3-clean/include/linux/sched.h	2014-09-17 11:22:16.000000000 -0600
+++ ./linux-3.16.3-vtmininet/include/linux/sched.h	2017-08-29 10:02:36.896974245 -0600
@@ -1406,6 +1406,16 @@ struct task_struct {
 	struct signal_struct *signal;
 	struct sighand_struct *sighand;
 
+    /* virtual time ---Jiaqi */
+    s64 virtual_start_time;
+    s64 physical_past_time;
+    s64 virtual_past_time;
+    int dilation;
+
+    s64 virtual_start_nsec;
+    s64 physical_past_nsec;
+    s64 virtual_past_nsec;
+
 	sigset_t blocked, real_blocked;
 	sigset_t saved_sigmask;	/* restored if set_restore_sigmask() was used */
 	struct sigpending pending;
