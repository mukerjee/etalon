# bin

Various runables:

- `arp_clear.sh`: Clears the ARP table.

- `arp_poison.sh`: Sets all physical host ARP entries to the switch host's MAC
  address, causing packets destined for the other physical hosts to be sent to
  the switch host instead.

- `click_startup.sh`: Launches the hybrid switch.

- `gen_switch.py`: Generates the hybrid switch specification. Executed by
  `click_startup.sh`.

- `install.sh`: The top-level script for configuring an Etalon physical
  host. Call this on every physical host in the testbed cluster.

- `kernel_install.sh`: A helper script that installs the updated kernel needed
  by reTCP. Executed by `install.sh`. Should not be executed directly.

- `node_install.sh`: A helper script that configures a regular Etalon
  host. Executed by `install.sh`. Should not be executed directly.

- `profile_etalon_ccanel.py`: A CloudLab profile that creates an Etalon cluster.

- `profile_etalon_ccanel_bootstrap.py`: A simple CloudLab profile for creating a
  disk image for use with the `profile_etalon_ccanel.py` profile.

- `retcp_install.sh`: A helper script that installs the reTCP kernel
  module. Configured by `install.sh` to run automatically on boot. Should not be
  executed directly.

- `s`: Logs into a physical host via SSH using its handle (e.g., `./s host1`
  will log into `host1`). Assumes that `../etc/handles` is configured properly.

- `switch_install.sh`: A helper script that configures an Etalon switch host.
  Executed by `install.sh`. Should not be executed directly.

- `sxp.py`: Copies an experiment results file from the switch host to the local
   host using `scp`. E.g., `./sxp.py 1519494787` looks on the switch host for an
   experiment (in `/etalon/experiments/\[buffers, adu, hdfs\]`) result file with
   the timestamp `1519494787` and copies it to the local host. Assumes that
   `../etc/handles` is configured properly.

- `tune.sh`: A helper script that sets IP addresses, does some tuning, and
  generates a proper `/etc/hosts` file. Assumes that hosts have hostnames
  "host1", "host2", etc. and that the switch has hostname "switch". Configured
  by `install.sh` to run automatically on boot.

- `utils.sh`: Utility functions used by the other scripts.

## Instructions for creating a new Etalon disk image on CloudLab:

***Note: These instructions are untested.***

1. Launch a dummy CloudLab cluster using the
   `profile_etalon_ccanel_bootstrap.py` profile. This will launch a simple
   one-node cluster. We will use this node as the basis for a new disk image.

2. Log in to the node, clone the Etalon repository into the home directory, and
   run `kernel_install.sh`. This will install the reTCP kernel patch and reboot
   the host.

3. Use the CloudLab web interface to create a new disk image based on this node.
