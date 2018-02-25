# bin

Various runables:

- ```arp_clear.sh```: clears the local machine's arp table.

- ```arp_poison.sh```: sets all physical host arp entries to the switch
  machine's MAC address.

- ```click_startup.sh```: launches the software switch.

- ```gen-switch.py```: generates the software switch configuration (called by
  ```click_startup.sh```).

- ```kernel_install.sh```: installs the updated kernel needed by reTCP. Doesn't
  need to be run if using the Etalon experiment profile on CloudLab.

- ```node_install.sh```: installs everything the non-switch physical machines
  need to run Etalon.

- ```s```: a wrapper around ssh that allows handle use (e.g., ```./s host1```
  will log into host1 if set in ```../etc/handles```.

- ```switch_install.sh```: installs everything the software switch physical
  machine needs to run Etalon.

- ```sxp.py```: a wrapper around scp that should be run on the local machine
  that takes in a timestamp (e.g., ```./sxp.py 1519494787```) and looks on the
  software switch machine (as set in ```../etc/handles```) for an experiment (in
  /etalon/experiments/\[buffers, adu, hdfs\]) result file with that timestamp,
  copying it to the local machine.

- ```tune.sh```: run at startup on all physical nodes (as installed into crontab
  by ```switch_install.sh``` and ```node_install.shh```) to set IP addresses of
  machines, do some tuning, and generate a proper /etc/hosts. Assumes hosts have
  hostnames "host1", "host2", etc. and that the switch has hostname "switch"
  