#!/bin/sh

$HADOOP_INSTALL/sbin/start-dfs.sh
$HADOOP_INSTALL/sbin/start-yarn.sh
$HADOOP_INSTALL/sbin/mr-jobhistory-daemon.sh start historyserver
