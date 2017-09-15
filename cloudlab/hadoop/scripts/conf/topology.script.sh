#!/bin/bash
HADOOP_CONF=/hadoop/conf
echo `date` input: $@ >> $HADOOP_CONF/topology.log
while [ $# -gt 0 ] ; do
  nodeArg=$1
  exec< ${HADOOP_CONF}/topology.data
  result=""
  while read line ; do
    ar=( $line )
    if [ "${ar[0]}" = "$nodeArg" ] ; then
      result="${ar[1]}"
    fi
  done
  shift
  if [ -z "$result" ] ; then
     echo -n "/rack01"
  else
    echo -n "$result "
  fi
done
