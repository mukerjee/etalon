#!/usr/bin/env python

ami = 'ami-9be6f38c'
key = 'daehyeok-workstation rsa'
sec_group = 'sg-5c12ee20'
instance_type = 'c4.8xlarge'
subnet = 'subnet-e8348db3'
elastic_id = 'eipalloc-3a833704'
elastic_ip = 'ec2-34-193-91-214.compute-1.amazonaws.com'
host_count = 4
group_name = 'DC'

spot = True
spot_price = 0.50  # 50 cents
