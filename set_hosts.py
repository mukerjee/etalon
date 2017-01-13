import os
import subprocess
import re

publicIP = {'switch':[], 'hosts':[]}
privateIP = {'switch':[], 'hosts':[]}

def init():
    switch_idx = -1
    cmd = ("aws ec2 describe-instances "
           "--query \"Reservations[*].Instances[*].PublicIpAddress\" "
           "--output=text")
    output = subprocess.check_output(cmd, shell=True)
    ip_list = re.split('\r|\t|\n', output)
    for i in range(0, len(ip_list)):
        if ip_list[i] == '':
            continue
        if ip_list[i].startswith('3'):
            publicIP['switch'] = [ip_list[i]]
            switch_idx = i
        else:
            publicIP['hosts'].append(ip_list[i])

    cmd = ("aws ec2 describe-instances "
           "--query \"Reservations[*].Instances[*].PrivateIpAddress\" "
           "--output=text")
    output = subprocess.check_output(cmd, shell=True)
    ip_list = re.split('\r|\t|\n', output)

    for i in range(0, len(ip_list)):
        if ip_list[i] == '':
            continue
        if i == switch_idx:
            privateIP['switch'] = [ip_list[i]]
        else:
            privateIP['hosts'].append(ip_list[i])

    fout = open('hosts.txt','w')

    for ip in privateIP['hosts']:
        fout.write(ip+'\n')
    fout.close()

    os.system('scp ./hosts.txt ec2-user@%s:~/' % (publicIP['switch']))

if __name__ == '__main__':
    init()


