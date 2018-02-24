# vhost/config/hadoop_config

Configuration files for hadoop/hdfs. The only important changes are:

1. a script that assigns nodes to racks, based on their IP.

2. reducing replication factor down to 2 (as a third replica will be on the
second's rack anyways.
