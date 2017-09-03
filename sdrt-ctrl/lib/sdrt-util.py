#! /usr/bin/python

import argparse
import os
import subprocess

parser = argparse.ArgumentParser()
parser.add_argument("-c", "--command", help="Command that you wish to run", nargs="+")
args = parser.parse_args()

new_env = os.environ
new_env["LD_PRELOAD"]    = "./sdrt-ctrl.so"

cmd = " ".join(args.command)
try:
    subprocess.call(cmd, env=new_env, shell=True)
except:
    exit(0)

