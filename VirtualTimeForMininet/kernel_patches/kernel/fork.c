--- linux-3.16.3-clean/kernel/fork.c	2014-09-17 11:22:16.000000000 -0600
+++ ./linux-3.16.3-vtmininet/kernel/fork.c	2017-08-29 10:02:36.880974424 -0600
@@ -2,8 +2,8 @@
  *  linux/kernel/fork.c
  *
  *  Copyright (C) 1991, 1992  Linus Torvalds
+ *
  */
-
 /*
  *  'fork.c' contains the help-routines for the 'fork' system call
  * (see also entry.S and others).
@@ -1567,6 +1567,54 @@ struct task_struct *fork_idle(int cpu)
 }
 
 /*
+ *	Initialize virtual start time as this moment
+ */
+static void init_virtual_start_time(struct task_struct *task, int dilation)
+{
+	// if(likely(dilation != 0)) 
+	if(dilation > 0) {
+
+	    /* just make sure further get(ns)timeofday return original time */
+	    // task->virtual_start_time = 0; 
+	    task->virtual_start_nsec = 0;
+		task->dilation = 0;
+	    /* initialize fields of time, old version virtual time */
+	    // struct timeval now;
+	    // do_gettimeofday(&now);
+	    // task->virtual_start_time = timeval_to_ns(&now);
+	    // task->physical_past_time = 0;
+	    // task->virtual_past_time = 0;
+	    // printk("[info] [process %d] created and started at %ld\n", task->pid, task->virtual_start_time);
+
+	    /* method 1: will return nano seconds since Epoch 1970 */
+	    struct timespec ts;
+	    getnstimeofday(&ts);
+		s64 getnstimeofday_nsec= timespec_to_ns(&ts);
+	    // printk("[info] [process %d] getnstimeofday virtual start time = %lld nano secs\n", task->pid, getnstimeofday_nsec);
+
+	    /* method 2: will return nano seconds since system boot 
+	    s64 ktime_nsec = ktime_to_ns(ktime_get());
+	    printk("[info] [process %d] ktime_get virtual start time = %lld nano secs\n", task->pid, ktime_nsec);
+		*/
+	    
+	    /* method 3: as the same as method 1 
+	    struct timeval tv;
+		do_gettimeofday(&tv);
+	    s64 do_gettimeofday_nsec = timeval_to_ns(&tv);
+	    printk("[info] [process %d] do_gettimeofday virtual start time = %lld\n", task->pid, do_gettimeofday_nsec);
+		*/
+	    
+	    /* initialize fields of nsec */
+		task->virtual_start_nsec = getnstimeofday_nsec;
+		// task->virtual_start_nsec = ktime_nsec;
+		// task->virtual_start_nsec = do_gettimeofday_nsec;
+	    task->physical_past_nsec = 0;
+	    task->virtual_past_nsec = 0;
+	    task->dilation = dilation;
+	}
+}
+
+/*
  *  Ok, this is the main fork-routine.
  *
  * It copies the process, and if successful kick-starts
@@ -1582,6 +1630,12 @@ long do_fork(unsigned long clone_flags,
 	int trace = 0;
 	long nr;
 
+    int dilation = 0;
+
+    if(clone_flags & CLONE_TIME) {
+        dilation = (int)stack_size;
+        stack_size = 0;
+    }
 	/*
 	 * Determine whether and which event to report to ptracer.  When
 	 * called from kernel_thread or CLONE_UNTRACED is explicitly
@@ -1602,6 +1656,7 @@ long do_fork(unsigned long clone_flags,
 
 	p = copy_process(clone_flags, stack_start, stack_size,
 			 child_tidptr, NULL, trace);
+
 	/*
 	 * Do this prior waking up the new thread - the thread pointer
 	 * might get invalid after that point, if the thread exits quickly.
@@ -1624,6 +1679,11 @@ long do_fork(unsigned long clone_flags,
 			get_task_struct(p);
 		}
 
+        if(clone_flags & CLONE_TIME) {
+
+            init_virtual_start_time(p, dilation);
+        }
+
 		wake_up_new_task(p);
 
 		/* forking complete and child started to run, tell ptracer */
@@ -1699,6 +1759,19 @@ SYSCALL_DEFINE5(clone, unsigned long, cl
 }
 #endif
 
+// /* virtual time ---Jiaqi */
+// SYSCALL_DEFINE5(timeclone, unsigned long, clone_flags, unsigned long, newsp, int __user *, parent_tidptr, int, tls_val, int __user*, child_tidptr)
+// {
+//     printk("[info] syscall timeclone called with with dilation value %d\n", tls_val);
+
+//     if((clone_flags & CLONE_TIME) && (tls_val != 0)) {
+//         return do_fork(clone_flags | SIGCHLD | 0x02000000, newsp, tls_val, parent_tidptr, child_tidptr);
+//     } else {
+//         return do_fork(clone_flags, newsp, 0, parent_tidptr, child_tidptr);
+//     }
+// }
+
+
 #ifndef ARCH_MIN_MMSTRUCT_ALIGN
 #define ARCH_MIN_MMSTRUCT_ALIGN 0
 #endif
@@ -1749,7 +1822,7 @@ static int check_unshare_flags(unsigned
 	if (unshare_flags & ~(CLONE_THREAD|CLONE_FS|CLONE_NEWNS|CLONE_SIGHAND|
 				CLONE_VM|CLONE_FILES|CLONE_SYSVSEM|
 				CLONE_NEWUTS|CLONE_NEWIPC|CLONE_NEWNET|
-				CLONE_NEWUSER|CLONE_NEWPID))
+				CLONE_NEWUSER|CLONE_NEWPID|CLONE_TIME))
 		return -EINVAL;
 	/*
 	 * Not implemented, but pretend it works if there is nothing to
@@ -1943,3 +2016,120 @@ int unshare_files(struct files_struct **
 	task_unlock(task);
 	return 0;
 }
+
+/*
+ * Initialize virtual time parameters in this system call --- Jiaqi
+ */
+SYSCALL_DEFINE2(virtualtimeunshare, unsigned long, unshare_flags, unsigned long, dilation)
+{
+    // printk("[info] syscall virtualtimeunshare called with with dilation value %d\n", dilation);
+
+	struct fs_struct *fs, *new_fs = NULL;
+	struct files_struct *fd, *new_fd = NULL;
+	struct cred *new_cred = NULL;
+	struct nsproxy *new_nsproxy = NULL;
+	int do_sysvsem = 0;
+	int err;
+
+	/*
+	 * If unsharing a user namespace must also unshare the thread.
+	 */
+	if (unshare_flags & CLONE_NEWUSER)
+		unshare_flags |= CLONE_THREAD | CLONE_FS;
+	/*
+	 * If unsharing a thread from a thread group, must also unshare vm.
+	 */
+	if (unshare_flags & CLONE_THREAD)
+		unshare_flags |= CLONE_VM;
+	/*
+	 * If unsharing vm, must also unshare signal handlers.
+	 */
+	if (unshare_flags & CLONE_VM)
+		unshare_flags |= CLONE_SIGHAND;
+	/*
+	 * If unsharing namespace, must also unshare filesystem information.
+	 */
+	if (unshare_flags & CLONE_NEWNS)
+		unshare_flags |= CLONE_FS;
+
+	err = check_unshare_flags(unshare_flags);
+	if (err)
+		goto bad_unshare_out;
+	/*
+	 * CLONE_NEWIPC must also detach from the undolist: after switching
+	 * to a new ipc namespace, the semaphore arrays from the old
+	 * namespace are unreachable.
+	 */
+	if (unshare_flags & (CLONE_NEWIPC|CLONE_SYSVSEM))
+		do_sysvsem = 1;
+	err = unshare_fs(unshare_flags, &new_fs);
+	if (err)
+		goto bad_unshare_out;
+	err = unshare_fd(unshare_flags, &new_fd);
+	if (err)
+		goto bad_unshare_cleanup_fs;
+	err = unshare_userns(unshare_flags, &new_cred);
+	if (err)
+		goto bad_unshare_cleanup_fd;
+	err = unshare_nsproxy_namespaces(unshare_flags, &new_nsproxy,
+					 new_cred, new_fs);
+	if (err)
+		goto bad_unshare_cleanup_cred;
+
+	if (new_fs || new_fd || do_sysvsem || new_cred || new_nsproxy) {
+		if (do_sysvsem) {
+			/*
+			 * CLONE_SYSVSEM is equivalent to sys_exit().
+			 */
+			exit_sem(current);
+		}
+
+		if (new_nsproxy)
+			switch_task_namespaces(current, new_nsproxy);
+
+		task_lock(current);
+
+		if (new_fs) {
+			fs = current->fs;
+			spin_lock(&fs->lock);
+			current->fs = new_fs;
+			if (--fs->users)
+				new_fs = NULL;
+			else
+				new_fs = fs;
+			spin_unlock(&fs->lock);
+		}
+
+		if (new_fd) {
+			fd = current->files;
+			current->files = new_fd;
+			new_fd = fd;
+		}
+
+		task_unlock(current);
+
+		/* Here we make virtual_start_time to be the current moment */
+		init_virtual_start_time(current, dilation);
+
+		if (new_cred) {
+			/* Install the new user namespace */
+			commit_creds(new_cred);
+			new_cred = NULL;
+		}
+	}
+
+bad_unshare_cleanup_cred:
+	if (new_cred)
+		put_cred(new_cred);
+bad_unshare_cleanup_fd:
+	if (new_fd)
+		put_files_struct(new_fd);
+
+bad_unshare_cleanup_fs:
+	if (new_fs)
+		free_fs_struct(new_fs);
+
+bad_unshare_out:
+	return err;
+}
+
