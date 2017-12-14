#!/usr/bin/python


import sys, os, string, threading
import paramiko

cmd = "~/bin/update_hadoop.sh"

outlock = threading.Lock()

def workon(host):
    ssh = paramiko.SSHClient()
    ssh.set_missing_host_key_policy(paramiko.AutoAddPolicy())
    ssh.connect(host)#, username='xy', password='xy')
    stdin, stdout, stderr = ssh.exec_command(cmd)
    stdin.write('xy\n')
    stdin.flush()

    with outlock:
        print stdout.readlines()

def main():
    hosts = ["ip-172-31-20-108","ip-172-31-20-110","ip-172-31-20-109"]
    threads = []
    for h in hosts:
        t = threading.Thread(target=workon, args=(h,))
        t.start()
        threads.append(t)
    for t in threads:
        t.join()

    os.system("./update_hadoop.sh")

main()
