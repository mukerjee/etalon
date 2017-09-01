--- linux-3.16.3-clean/include/linux/syscalls.h	2014-09-17 11:22:16.000000000 -0600
+++ ./linux-3.16.3-vtmininet/include/linux/syscalls.h	2017-08-29 10:02:36.900974200 -0600
@@ -831,6 +831,14 @@ asmlinkage long sys_clone(unsigned long,
 #endif
 #endif
 
+/* virtual time --- Jiaqi*/
+// asmlinkage long sys_timeclone(unsigned long, unsigned long, int __user *, int, int __user *);
+asmlinkage long sys_virtualtimeunshare(unsigned long, unsigned long);
+asmlinkage long sys_getvirtualtimeofday(struct timeval __user *, struct timezone __user *);
+asmlinkage long sys_helloworld(void);
+asmlinkage long sys_settimedilationfactor(unsigned long, pid_t);
+
+
 asmlinkage long sys_execve(const char __user *filename,
 		const char __user *const __user *argv,
 		const char __user *const __user *envp);
