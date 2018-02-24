# reHDFS (Reconfigurable Datacenter Network HDFS)

reHDFS is modified HDFS block placement policy (i.e., it tells HDFS where to
place replicas on write). reHDFS' policy is simple, given a writer on rack 1,
place the replica in rack 2. Writers in rack 2 place replicas in rack 3,
etc. When compared with the default policy (which chooses random racks), reHDFS
creates a ring workload, as opposed to an all-to-all workload.