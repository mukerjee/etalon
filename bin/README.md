# bin

Various runables:

- ```arp_clear.sh```: clears the local machine's arp table.

- ```arp_poison.sh```: sets all physical host arp entries to the switch
  machine's MAC address.

- ```click_startup.sh```: launches the software switch.

- ```gen-switch.py```: generates the software switch configuration (called by
  ```click_startup.sh```).

- ```kernel_install.sh```: installs the updated kernel needed by reTCP. Doesn't
  need to be run if using the Etalon experiment profile on CloudLab. Assumes
  that the Etalon repository is in your home directory.

- ```node_install.sh```: installs everything the non-switch physical machines
  need to run Etalon.

- ```profile_etalon-ccanel.py```: A CloudLab profile that creates an Etalon
  cluster.

- ```profile_etalon-ccanel-bootstrap.py```: A simple CloudLab profile for
  creating a disk image for use with the ```profile_etalon-ccanel.py``` profile.

- ```s```: a wrapper around ssh that allows handle use (e.g., ```./s host1```
  will log into host1 if set in ```../etc/handles```.

- ```switch_install.sh```: installs everything the software switch physical
  machine needs to run Etalon.

- ```sxp.py```: a wrapper around scp that should be run on the local machine
  that takes in a timestamp (e.g., ```./sxp.py 1519494787```) and looks on the
  software switch machine (as set in ```../etc/handles```) for an experiment (in
  ```/etalon/experiments/\[buffers, adu, hdfs\]```) result file with that
  timestamp, copying it to the local machine.

- ```config_on_boot.sh```: run manually at startup on all physical nodes to set IP addresses of
  machines, do some tuning, and generate a proper /etc/hosts. Assumes hosts have
  hostnames "host1", "host2", etc. and that the switch has hostname "switch".

## Instructions for creating a new Etalon disk image on CloudLab:

***Note: These instructions are untested.***

1. Launch a dummy CloudLab cluster using the
   ```profile_etalon-ccanel-bootstrap.py``` profile. This will launch a simple
   one-node cluster. We will use this node as the basis for the new disk image.

2. Log in to the node, clone the Etalon repository into the home directory, and
   run the script: ```~/etalon/bin/kernel_install.sh```. This will install the
   reTCP kernel patch and reboot the machine.

3. Use the CloudLab web interface to create a new disk image based on this node.
