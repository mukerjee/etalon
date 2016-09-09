#!/usr/bin/python

import sys
import subprocess
import json
import os
import time
import base64

# create an EC2 instance at a given region
# usage: launch_switch.py [type]

ami = 'ami-6869aa05'
key = 'mukerjee-mba rsa'
sec_group = 'sg-13971669'
data_file = 'startup_switch.sh'
instance_type = 't2.micro'
subnet = 'subnet-54415920'
elastic_id = 'eipalloc-31a9130e'
group_name = 'DC'

spot_price = 0.50  # 50 cents


def launch_switch(it, spot=False):
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
        open("/tmp/configuration_switch_spot.json", 'w').write(
            json.dumps(spot_config))
        cmd = ("aws ec2 request-spot-instances "
               "--spot-price \"" + str(spot_price) + "\" "
               "--instance-count 1 "
               "--type \"one-time\" "
               "--launch-group DC "
               "--launch-specification "
               "file:///tmp/configuration_switch_spot.json ")
    else:
        cmd = ("aws ec2 run-instances "
               "--image-id \"" + ami + "\" "
               "--key-name \"" + key + "\" "
               "--security-group-ids \"" + sec_group + "\" "
               "--user-data \"file://" + data_file + "\" "
               "--instance-type \"" + it + "\" "
               "--subnet-id \"" + subnet + "\" "
               "--count 1 ")
        if 't2' not in it:
            cmd += "--placement GroupName=\"" + group_name + "\" "

    print cmd
    instance_data = json.loads(subprocess.check_output(cmd, shell=True))

    if spot:
        spot_id = instance_data['SpotInstanceRequests'][0][
            'SpotInstanceRequestId']

        state = 'stopped'
        time.sleep(30)

        while state != 'running':
            instance_data = json.loads(subprocess.check_output(
                "aws ec2 describe-instances "
                "--filters Name=spot-instance-request-id,Values="
                + spot_id, shell=True))

            for set in instance_data['Reservations']:
                for node in set['Instances']:
                    state = node['State']['Name']
                    id = node['InstanceId']
            time.sleep(1)
        
    else:
        id = instance_data['Instances'][0]['InstanceId']

        state = 'stopped'
        time.sleep(10)

        # wait until instance is running
        while state != 'running':
            instance_data = json.loads(subprocess.check_output(
                'aws ec2 describe-instances --instance-ids ' + id, shell=True))

            for set in instance_data['Reservations']:
                for node in set['Instances']:
                    state = node['State']['Name']
            time.sleep(1)

    # name the switch
    name = "DC - Switch"
    cmd = ("aws ec2 create-tags "
           "--resources \"" + id + "\" "
           "--tags Key=Name,Value=\"" + name + "\" ")
    print cmd
    os.system(cmd)

    # turn off source/dest checking
    cmd = ("aws ec2 modify-instance-attribute "
           "--instance-id \"" + id + "\" "
           "--no-source-dest-check")
    print cmd
    os.system(cmd)

    # associate elastic ip
    cmd = ("aws ec2 associate-address "
           "--instance-id \"" + id + "\" "
           "--allocation-id \"" + elastic_id + "\"")
    print cmd
    subprocess.check_output(cmd, shell=True)

    
if __name__ == '__main__':
    it = sys.argv[1] if len(sys.argv) > 1 else instance_type
    launch_switch(it)
