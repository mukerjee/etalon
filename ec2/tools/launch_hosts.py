#!/usr/bin/python

import sys
import subprocess
import json
import base64

from config import ami, key, sec_group, instance_type, subnet, \
    host_count, group_name, spot, spot_price

import startup_hosts as data_file

# create an EC2 instance at a given region
# usage: launch_hosts.py [count] [type] [spot]


def launch_hosts(it, c, spot=False):
    if spot:
        spot_config = {
            "ImageId": ami,
            "KeyName": key,
            "SecurityGroupIds": [sec_group],
            "UserData": base64.b64encode(data_file.read()),
            "InstanceType": it,
            "SubnetId": subnet,
            "Placement": {
                "GroupName": group_name
            },
            "BlockDeviceMappings": [
                {
                    "DeviceName" : "/dev/xvda",
                     "Ebs" : { "VolumeSize" : 50 }
                }
            ]

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
               "--user-data \"file://" + data_file.dump_file() + "\" "
               "--instance-type \"" + it + "\" "
               "--subnet-id \"" + subnet + "\" "
               "--count \"" + str(c) + "\" ")
        if 't2' not in it:
            cmd += "--placement GroupName=\"" + group_name + "\" "
    print cmd
    subprocess.check_output(cmd, shell=True)

if __name__ == '__main__':
    c = int(sys.argv[1]) if len(sys.argv) > 1 else host_count
    it = sys.argv[2] if len(sys.argv) > 2 else instance_type
    sp = sys.argv[3] if len(sys.argv) > 3 else spot
    launch_hosts(it, c, sp)
