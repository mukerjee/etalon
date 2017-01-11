#!/usr/bin/env python

ami = 'ami-6869aa05'
key = 'daehyeok-workstation rsa'
sec_group = 'sg-5c12ee20'
instance_type = 't2.micro'
subnet = 'subnet-e8348db3'
elastic_id = 'eipalloc-3a833704'
elastic_ip = 'ec2-34-193-91-214.compute-1.amazonaws.com'
host_count = 2
group_name = 'DC'

spot = False
spot_price = 0.50  # 50 cents
