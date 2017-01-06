#!/usr/bin/env python

ami = 'ami-6869aa05'
key = 'daehyeok-workstation rsa'
sec_group = 'sg-13971669'
instance_type = 'c4.8xlarge'
subnet = 'subnet-54415920'
elastic_id = 'eipalloc-bf0cd381'
elastic_ip = 'ec2-34-196-30-45.compute-1.amazonaws.com'
host_count = 2
group_name = 'DC'

spot = True
spot_price = 0.50  # 50 cents
