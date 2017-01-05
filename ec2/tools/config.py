#!/usr/bin/env python

ami = 'ami-6869aa05'
key = 'daehyeok-workstation rsa'
sec_group = 'sg-13971669'
instance_type = 't2.micro'
subnet = 'subnet-54415920'
elastic_id = 'eipalloc-31a9130e'
elastic_ip = 'ec2-54-164-9-196.compute-1.amazonaws.com'
host_count = 3
group_name = 'DC'

spot = True
spot_price = 0.50  # 50 cents
