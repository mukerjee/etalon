#!/usr/bin/python

import subprocess
import os
import json


def describe_running_instances():
    # spits out a handles file using describe_ec2.py as input

    cmd = 'aws ec2 describe-instances'
    print cmd
    instance_data = json.loads(subprocess.check_output(cmd, shell=True))

    dns_names = {}
    names = {}

    for set in instance_data['Reservations']:
        for node in set['Instances']:
            if node['State']['Name'] == 'running':
                dns_names[node['InstanceId']] = node['PublicDnsName']
            if node['State']['Name'] == 'terminated':
                continue
            if 'Tags' in node:
                for t in node['Tags']:
                    if t['Key'] == 'Name':
                        names[node['InstanceId']] = t['Value']

    host_max = 0
    for n in names.values():
        if 'H' in n:
            host_max = max(host_max, int(n.split(' - ')[1][1:]))

    output = ""
    for i in dns_names.keys():
        if i not in names.keys():
            hn = host_max + 1
            name = "DC - H" + str(hn)
            cmd = ("aws ec2 create-tags "
                   "--resources \"" + i + "\" "
                   "--tags Key=Name,Value=\"" + name + "\" ")
            print cmd
            os.system(cmd)
            host_max += 1
            names[i] = name
        output += '%s #%s\n' % (names[i].split(" - ")[1].lower(), dns_names[i])

    open("../common/handles.ec2", 'w').write(output)

if __name__ == '__main__':
    describe_running_instances()
