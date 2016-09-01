#!/usr/bin/python

import subprocess
import os


def main():
    # spits out a handles file using describe_ec2.py as input

    lines = subprocess.check_output('ec2din --region us-east-1', shell=True)
    lines = lines.split('\n')

    dns_names = {}
    names = {}

    for line in lines:
        if 'INSTANCE' in line and 'running' in line:
            l = line.split()
            dns_names[l[1]] = l[3]
        elif 'TAG' in line:
            l = line.split()
            if l[3] == 'Name':
                names[l[2]] = ' '.join(l[4:])

    host_max = 1
    for n in names.values():
        if 'H' in n:
            host_max = max(host_max, int(n.split(' - ')[1][1:]))

    output = ""
    for i in dns_names.keys():
        if i not in names.keys():
            hn = host_max + 1
            cmd = ("ec2-create-tags "
                   "--resources \"" + i + "\" "
                   "--tags Key=Name,Value=\"DC - H" + hn + "\" ")
            print cmd
            os.system(cmd)
            host_max += 1
        output += '%s #%s\n' % (names[i].split(" - ")[1].lower(), dns_names[i])

    open("../common/handles.ec2", 'w').write(output)

if __name__ == '__main__':
    main()
