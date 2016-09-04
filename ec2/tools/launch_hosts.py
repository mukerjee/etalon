#!/usr/bin/python

import sys
import subprocess
import json
import base64

# create an EC2 instance at a given region
# usage: launch_hosts.py [count] [type]

ami = 'ami-6869aa05'
key = 'mukerjee-mba rsa'
sec_group = 'sg-13971669'
data_file = 'startup_hosts.sh'
instance_type = 'm4.large'
subnet = 'subnet-54415920'
count = 1
group_name = 'DC'

spot_price = 0.50  # 50 cents


def launch_hosts(it, c, spot=False):
    if spot:
        spot_config = {
            "ImageId": ami,
            "KeyName": key,
            "SecurityGroupIds": [sec_group],
            "UserData": base64.b64encode(open(data_file).read()),
            "InstanceType": it,
            "SubnetId": subnet,
            "Placement": {
                "GroupName": group_name
            }
        }
        open("/tmp/configuration_hosts_spot.json", 'w').write(
            json.dumps(spot_config))
        cmd = ("aws ec2 request-spot-instances "
               "--spot-price \"" + str(spot_price) + "\" "
               "--instance-count \"" + str(c) + "\" "
               "--type \"one-time\" "
               "--launch-group DC "
               "--launch-specification "
               "file:///tmp/configuration_hosts_spot.json ")
    else:
        cmd = ("aws ec2 run-instances "
               "--image-id \"" + ami + "\" "
               "--key-name \"" + key + "\" "
               "--security-group-ids \"" + sec_group + "\" "
               "--user-data \"file://" + data_file + "\" "
               "--instance-type \"" + it + "\" "
               "--subnet-id \"" + subnet + "\" "
               "--count \"" + str(c) + "\" ")
        if 't2' not in it:
            cmd += "--placement GroupName=\"" + group_name + "\" "
    print cmd
    subprocess.check_output(cmd, shell=True)

if __name__ == '__main__':
    c = int(sys.argv[1]) if len(sys.argv) > 1 else count
    it = sys.argv[2] if len(sys.argv) > 2 else instance_type
    launch_hosts(it, c)
