--- linux-3.16.3-clean/kernel/time.c	2014-09-17 11:22:16.000000000 -0600
+++ ./linux-3.16.3-vtmininet/kernel/time.c	2017-08-29 10:02:36.884974380 -0600
@@ -37,7 +37,7 @@
 #include <linux/fs.h>
 #include <linux/math64.h>
 #include <linux/ptrace.h>
-
+#include <linux/rcupdate.h> // for accessing current's parent
 #include <asm/uaccess.h>
 #include <asm/unistd.h>
 
@@ -98,12 +98,121 @@ SYSCALL_DEFINE1(stime, time_t __user *,
 
 #endif /* __ARCH_WANT_SYS_TIME */
 
+
 SYSCALL_DEFINE2(gettimeofday, struct timeval __user *, tv,
 		struct timezone __user *, tz)
 {
-	if (likely(tv != NULL)) {
+	// printk("[info] [process %d] in normal gettimeofday\n", current->pid);
+    if (likely(tv != NULL)) {
+		struct timeval ktv;
+		do_gettimeofday(&ktv);
+
+		if (copy_to_user(tv, &ktv, sizeof(ktv)))
+			return -EFAULT;
+	}
+	if (unlikely(tz != NULL)) {
+		if (copy_to_user(tz, &sys_tz, sizeof(sys_tz)))
+			return -EFAULT;
+	}
+	return 0;
+}
+
+/*
+SYSCALL_DEFINE2(gettimeofday, struct timeval __user *, tv, struct timezone __user *, tz)
+{
+    if (likely(tv != NULL)) {
+		struct timeval ktv;
+		do_gettimeofday(&ktv);
+
+
+        if(current->virtual_start_time != 0) {
+
+            s64 now = timeval_to_ns(&ktv);
+            s64 physical_past_time;
+            s64 virtual_past_time;
+            s32 remainder;
+            int dilation = current->dilation;
+            physical_past_time = now - current->virtual_start_time;
+
+            // printk("[info] [process %d] gettimeofday is using dilation %d\n", current->pid, dilation);
+
+            // printk("[info] [process %d] physical past %ld nanosec from start\n", current->pid, physical_past_time);
+            if(dilation > 0) {
+                s64 dividend = physical_past_time - current->physical_past_time;
+                s64 divisor = div_u64_rem(dividend, dilation, &remainder);
+                // dilation is multiplied by 1000 when used as input
+                virtual_past_time = divisor * 1000 + current->virtual_past_time;
+                now = virtual_past_time + current->virtual_start_time;
+                // printk("[info] [process %d] %ld / %d = %ld ... %ld\n", current->pid, dividend, dilation, divisor, remainder);
+            } else {
+                virtual_past_time = (physical_past_time - current->physical_past_time) + current->virtual_past_time;
+                now = virtual_past_time + current->virtual_start_time;
+            }
+            // printk("[info] [process %d] virtual past %ld nanosec from start\n", current->pid, virtual_past_time);
+
+            ktv = ns_to_timeval(now);
+            current->physical_past_time = physical_past_time;
+            current->virtual_past_time = virtual_past_time;
+        } else {
+        	printk("[info] [process %d] gettimeofday is not using dilation", current->pid);
+        }
+
+		if (copy_to_user(tv, &ktv, sizeof(ktv)))
+			return -EFAULT;
+	}
+	if (unlikely(tz != NULL)) {
+		if (copy_to_user(tz, &sys_tz, sizeof(sys_tz)))
+			return -EFAULT;
+	}
+	return 0;
+}
+*/
+
+/*
+ *  Virtual past time = physical past time / dilation
+ *	Do any other things that normal gettimeofday does
+ *  getvirtualtimeofday --- Jiaqi Shoud not call this function now that gettimeofday can be captured
+ */
+SYSCALL_DEFINE2(getvirtualtimeofday, struct timeval __user *, tv,
+		struct timezone __user *, tz)
+{
+	// printk("[info] [process %d] entry to getvirtualtimeofday syscall\n", current->pid);
+    if (likely(tv != NULL)) {
 		struct timeval ktv;
 		do_gettimeofday(&ktv);
+
+
+        if(current->virtual_start_time != 0) {
+
+            s64 now = timeval_to_ns(&ktv);
+            s64 physical_past_time;
+            s64 virtual_past_time;
+            s32 remainder;
+            int dilation = current->dilation;
+            physical_past_time = now - current->virtual_start_time;
+            // printk("[info] [process %d] getvirtualtimeofday is using dilation %d\n", current->pid, dilation);
+
+            // printk("[info] [process %d] physical past %ld nanosec from start\n", current->pid, physical_past_time);
+            if(dilation > 0) {
+                s64 dividend = physical_past_time - current->physical_past_time;
+                s64 divisor = div_u64_rem(dividend, dilation, &remainder);
+                // dilation is multiplied by 1000 when used as input
+                virtual_past_time = divisor + current->virtual_past_time;
+                now = virtual_past_time + current->virtual_start_time;
+                // printk("[info] [process %d] %ld / %d = %ld ... %ld\n", current->pid, dividend, dilation, divisor, remainder);
+            } else {
+                virtual_past_time = (physical_past_time - current->physical_past_time) + current->virtual_past_time;
+                now = virtual_past_time + current->virtual_start_time;
+            }
+            // printk("[info] [process %d] virtual past %ld nanosec from start\n", current->pid, virtual_past_time);
+
+            ktv = ns_to_timeval(now);
+            current->physical_past_time = physical_past_time;
+            current->virtual_past_time = virtual_past_time;
+        } else {
+            printk("[info] [process %d] getvirtualtimeofday is not using time dilation", current->pid);
+        }
+
 		if (copy_to_user(tv, &ktv, sizeof(ktv)))
 			return -EFAULT;
 	}
@@ -115,6 +224,87 @@ SYSCALL_DEFINE2(gettimeofday, struct tim
 }
 
 /*
+ * Change current process's time dilation factor --- Jiaqi Yan
+ * What below is a total copy of init_virtual_start_time in fork.c
+ */
+SYSCALL_DEFINE2(settimedilationfactor, unsigned long, dilation, pid_t, ppid)
+{
+	if (dilation > 0) {
+
+		struct timeval now;
+		struct timespec ts;
+
+		if (ppid == 0) // change current's dilation
+		{
+			/* reset virtual_start so that we can get original time */
+			current->dilation = 0;
+
+			/* reset time fields, old version */
+			// current->virtual_start_time = 0;
+			// do_gettimeofday(&now);
+			// current->virtual_start_time = timeval_to_ns(&now);
+			// current->physical_past_time = 0;
+			// current->virtual_past_time = 0;
+
+			/* reset nsec fields, current version */
+			current->virtual_start_nsec = 0;
+			getnstimeofday(&ts);
+			current->virtual_start_nsec = timespec_to_ns(&ts);
+			current->physical_past_nsec = 0;
+			current->virtual_past_nsec = 0;
+
+			current->dilation = dilation;
+
+			// printk("[info] [process %d] in settimedilationfactor: change/set dilation to %ld\n", current->pid, current->dilation);
+			// printk("[info] [process %d] in settimedilationfactor: virtual RESTART time = %lld nano secs\n", current->pid, current->virtual_start_nsec);
+
+		} else { // change current's parent's dilation
+
+			/*
+			 * To access parent's data, we need to lock and unlock;
+			 * see https://www.kernel.org/doc/Documentation/RCU/whatisRCU.txt for more details
+			 */
+			rcu_read_lock();
+
+			struct task_struct* parent = rcu_dereference(current->real_parent);
+
+			/* pid_t == __kernel_pid_t == int */
+			if (!parent || parent->pid != ppid)
+			{
+				rcu_read_unlock();
+				return -1;
+			}
+
+			parent->dilation = 0;
+
+			/* reset virtual_start so that we can get original time */
+
+			/* reset time fields, old version */
+			// parent->virtual_start_time = 0;
+			// do_gettimeofday(&now);
+			// parent->virtual_start_time = timeval_to_ns(&now);
+			// parent->physical_past_time = 0;
+			// parent->virtual_past_time = 0;
+
+			/* reset nsec fields, current version */
+			parent->virtual_start_nsec = 0;
+			getnstimeofday(&ts);
+			parent->virtual_start_nsec = timespec_to_ns(&ts);
+			parent->physical_past_nsec = 0;
+			parent->virtual_past_nsec = 0;
+
+			parent->dilation = dilation;
+
+			// printk("[info] [process %d] in settimedilationfactor: change/set process %d's dilation to %ld\n", current->pid, parent->pid, parent->dilation);
+			// printk("[info] [process %d] in settimedilationfactor: process %d's virtual RESTART time = %lld nano secs\n", current->pid, parent->pid, parent->virtual_start_nsec);
+
+			rcu_read_unlock();
+		}
+	}
+	return 0;
+}
+
+/*
  * Indicates if there is an offset between the system clock and the hardware
  * clock/persistent clock/rtc.
  */
@@ -396,7 +586,7 @@ struct timespec ns_to_timespec(const s64
 		ts.tv_sec--;
 		rem += NSEC_PER_SEC;
 	}
-	ts.tv_nsec = rem;
+    ts.tv_nsec = rem;
 
 	return ts;
 }
