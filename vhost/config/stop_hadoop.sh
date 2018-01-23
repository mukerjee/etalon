#!/bin/sh

$HADOOP_INSTALL/sbin/stop-dfs.sh
$HADOOP_INSTALL/sbin/stop-yarn.sh
$HADOOP_INSTALL/sbin/mr-jobhistory-daemon.sh stop historyserver
