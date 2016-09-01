#!/usr/bin/python

import sys
import os

# create an EC2 instance at a given region
# usage: launch_ec2.py [count] [type]

ami = 'ami-6869aa05'
key = 'mukerjee-mba rsa'
sec_group = 'DC'
data_file = 'ec2_startup.sh'
type = 't2.micro'
subnet = 'subnet-54415920'


def main():
    count = 1
    if len(sys.argv) > 2:
        count = int(sys.argv[2])
    if len(sys.argv) > 3:
        type = sys.argv[3]

    cmd = ("ec2-run-instances "
           "--image-id \"" + ami + "\" "
           "--key-name \"" + key + "\" "
           "--security-groups \"" + sec_group + "\" "
           "--user-data-file \"" + data_file + "\" "
           "--instance-type \"" + type + "\" "
           "--subnet-id \"" + subnet + "\" "
           "--count \"" + count + "\" ")
    print cmd
    os.system(cmd)

if __name__ == '__main__':
    main()
