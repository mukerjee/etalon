#!/usr/bin/env python

import time
import sys
from launch_switch import launch_switch
from launch_hosts import launch_hosts
from describe_running_instances import describe_running_instances

instance_type = 't2.micro'  # 'c4.8xlarge'
host_count = 3

spot = True
if len(sys.argv) > 1 and sys.argv[1] == 'demand':
    spot = False

print 'creating switch...'
launch_switch(instance_type, spot)

time.sleep(120)
print 'creating hosts...'
launch_hosts(instance_type, host_count, spot)

time.sleep(60)
print 'describing instances...'
describe_running_instances()
