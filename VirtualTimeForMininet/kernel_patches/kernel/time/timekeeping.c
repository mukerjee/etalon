--- linux-3.16.3-clean/kernel/time/timekeeping.c	2014-09-17 11:22:16.000000000 -0600
+++ ./linux-3.16.3-vtmininet/kernel/time/timekeeping.c	2017-09-01 12:53:48.450976718 -0600
@@ -19,10 +19,12 @@
 #include <linux/clocksource.h>
 #include <linux/jiffies.h>
 #include <linux/time.h>
+#include <uapi/linux/time.h>
 #include <linux/tick.h>
 #include <linux/stop_machine.h>
 #include <linux/pvclock_gtod.h>
 #include <linux/compiler.h>
+#include <linux/math64.h> // for 64bit div
 
 #include "tick-internal.h"
 #include "ntp_internal.h"
@@ -290,6 +292,33 @@ static void timekeeping_forward_now(stru
 	timespec_add_ns(&tk->raw_time, nsec);
 }
 
+/* Delta virtual time = delta physical time / TDF */
+static void do_dilatetimeofday(struct timespec *ts)
+{
+  int dilation = current->dilation;
+  if(dilation > 0 && current->virtual_start_nsec > 0) {
+    struct timespec dilated_ts;
+
+    u64 now = timespec_to_ns(ts);
+    u64 physical_past_nsec = now - current->virtual_start_nsec;
+    u64 virtual_past_nsec = physical_past_nsec / dilation;
+    u64 dilated_now = virtual_past_nsec + current->virtual_start_nsec;
+
+    if (dilated_now > now) {
+      printk("[panic] [process %d] VT faster RT when dilation = %d (dilated_now = %lld > now = %lld\n)", 
+	     current->pid, dilation, dilated_now, now);
+      return;
+    }
+
+    dilated_ts = ns_to_timespec(dilated_now);
+
+    ts->tv_sec = dilated_ts.tv_sec;
+    ts->tv_nsec = dilated_ts.tv_nsec;
+    current->physical_past_nsec = physical_past_nsec;
+    current->virtual_past_nsec = virtual_past_nsec;
+  }
+}
+
 /**
  * __getnstimeofday - Returns the time of day in a timespec.
  * @ts:		pointer to the timespec to be set
@@ -320,6 +349,15 @@ int __getnstimeofday(struct timespec *ts
 	 */
 	if (unlikely(timekeeping_suspended))
 		return -EAGAIN;
+
+	/* Dilate time */
+	// s64 now = timespec_to_ns(ts);
+	// printk("[info] getnstimeofday before do_dilatetimeofday: %ld nano seconds\n", now);
+
+    // now = timespec_to_ns(ts);
+	// printk("[info] getnstimeofday after do_dilatetimeofday: %ld nano seconds\n", now);
+	do_dilatetimeofday(ts);
+
 	return 0;
 }
 EXPORT_SYMBOL(__getnstimeofday);
@@ -355,6 +393,18 @@ ktime_t ktime_get(void)
 	 * 32-bit architectures without CONFIG_KTIME_SCALAR.
 	 */
 	return ktime_add_ns(ktime_set(secs, 0), nsecs);
+
+    /* Dilated time
+    struct timespec ts = ktime_to_timespec(ktime_add_ns(ktime_set(secs, 0), nsecs));
+
+	// s64 now = timespec_to_ns(&ts);
+	// printk("[info] ktime_get before do_dilatetimeofday: %ld nano seconds\n", now);
+    // do_dilatetimeofday(&ts);
+    // now = timespec_to_ns(&ts);
+	// printk("[info] ktime_get after do_dilatetimeofday: %ld nano seconds\n", now);
+
+    return timespec_to_ktime(ts);
+    */
 }
 EXPORT_SYMBOL_GPL(ktime_get);
 
