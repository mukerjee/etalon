# VT-Mininet: Virtual Time Enabled Mininet for SDN Emulation

## Installation
### Install Virtual Time Enabled Kernel
All the modification to kernel code are under directory "kernel_changes". Since I was using version 3.16.3, a patch based on that version is provided, as "VirtualTime.patch". 
Alternative way to modify and build virtual time enabled kernel:


* Download kernel 3.16.3 from [The Linux Kernel Archives](https://www.kernel.org/pub/linux/kernel/v3.x/linux-3.16.3.tar.gz). Unzip it.
```
tar -zxvf linux-3.16.3.tar.gz
```
* Under "kernel_changes" directory, run following script so that source files under kernel_changes will replace orignal kernel code
```
./transfer.sh /Path/To/UnzippedKernel
```
* Change directory to unzipped kernel
	* You may need to configure kernel first. A easy way is to use existing configure file in your system
	```
	cp -vi /boot/config-`uname -r` .config
	yes "" | make oldconfig
	```
	* To compile kernel, modules and install them, you can run my script
	```
	sudo ./build_all.sh
	```
    * You may need to configure GRUB so that you can select which kernel to boot
    ```
    sudo vim /etc/default/grub
    GRUB_HIDDEN_TIMEOUT=15
    GRUB_HIDDEN_TIMEOUT_QUIET=false
    update-grub
    ```
    * Reboot and select the right kernel image in grub menu.


### Install Virtual Time Enabled Mininet
Since original Mininet-Hifi is keeping evolving and I forget which exact commit I was working on when developing virtual time, I do not think do an automatic patch here is very helpful (although I can guarantee you that my VT-Mininet is based at least on Mininet 2.1.0). Here I just provide full source of VT-Mininet. Since Mininet itself depends on many softwares:
```
OpenFlow, Open vSwitches
gcc, make, socat, psmisc, xterm, ssh, iperf3/iperf, iproute, telnet 
python-setuptools, cgroup-bin, ethtool, help2man, pyflakes, pylint, pep8
```
I recommend you take a look at mininet/util/install.sh and resolve the dependency first. A easier way is to first install original Mininet, them replace it with my VT-Mininet code and reinstall by running
```
sudo make clean
sudo make install
```
Tips: you may need to repeat clean-install twice.


## Acknowledge
My implementation of virtual time is actually inspired by that of Jeremy Lamps. Great thanks for sharing his source code and providing a very detailed documentation.

## Experiments with Virtual Time
coming soon...
