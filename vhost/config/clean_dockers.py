#!/usr/bin/python

import os

for i in range(1, 9):
    cmd = "ssh mukerjee@host%d /users/mukerjee/sdrt/vhost/config/clean_docker_img.sh"
    os.system(cmd)
