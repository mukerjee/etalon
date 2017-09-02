--- linux-3.16.3-clean/include/uapi/linux/sched.h	2014-09-17 11:22:16.000000000 -0600
+++ ./linux-3.16.3-vtmininet/include/uapi/linux/sched.h	2017-08-29 10:02:36.892974290 -0600
@@ -21,6 +21,8 @@
 #define CLONE_DETACHED		0x00400000	/* Unused, ignored */
 #define CLONE_UNTRACED		0x00800000	/* set if the tracing process can't force CLONE_PTRACE on this clone */
 #define CLONE_CHILD_SETTID	0x01000000	/* set the TID in the child */
+#define CLONE_TIME          0x02000000  /* virtual time support --- Jiaqi */
+
 /* 0x02000000 was previously the unused CLONE_STOPPED (Start in stopped state)
    and is now available for re-use. */
 #define CLONE_NEWUTS		0x04000000	/* New utsname group? */
