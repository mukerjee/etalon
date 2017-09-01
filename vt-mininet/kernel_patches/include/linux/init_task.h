--- linux-3.16.3-clean/include/linux/init_task.h	2014-09-17 11:22:16.000000000 -0600
+++ ./linux-3.16.3-vtmininet/include/linux/init_task.h	2017-08-29 10:02:36.900974200 -0600
@@ -195,7 +195,14 @@ extern struct task_group root_task_group
 	.ptraced	= LIST_HEAD_INIT(tsk.ptraced),			\
 	.ptrace_entry	= LIST_HEAD_INIT(tsk.ptrace_entry),		\
 	.real_parent	= &tsk,						\
-	.parent		= &tsk,						\
+    .virtual_start_time = 0,                    \
+    .physical_past_time = 0,                    \
+    .virtual_past_time = 0,                     \
+    .dilation = 0,                              \
+    .virtual_start_nsec = 0,                    \
+    .physical_past_nsec = 0,                    \
+    .virtual_past_nsec = 0,                     \
+    .parent		= &tsk,						\
 	.children	= LIST_HEAD_INIT(tsk.children),			\
 	.sibling	= LIST_HEAD_INIT(tsk.sibling),			\
 	.group_leader	= &tsk,						\
