# Etalon

Etalon is a reconfigurable datacenter network (RDCN) emulator designed to run on
public testbeds. We define an RDCN to be a datacenter network designed around
adding and removing circuits in realtime to add bandwidth to portions of the
network on demand. This has been seen in many research papers with a variety of
technologies: e.g., 60GHz wireless
([Flyways](https://dl.acm.org/citation.cfm?id=2018442)), optical circuit
switching ([c-Through](https://dl.acm.org/citation.cfm?id=1851222),
[Helios](https://dl.acm.org/citation.cfm?id=1851223),
[Mordia](https://dl.acm.org/citation.cfm?id=2486007), etc.), or free-space
optics ([FireFly](https://dl.acm.org/citation.cfm?id=2626328),
[ProjecToR](https://dl.acm.org/citation.cfm?id=2934911)). Some of these
proposals incorporate a packet switch, which we model as well. With Etalon, 
we wish to provide a platform where repeatable end-to-end
experiments on RDCN can be emulated at scale.


## Overview

Etalon emulates a datacenter by running multiple virtual hosts ("vhosts"; Docker
containers) on different physical machines. Physical machines represent
datacenter "racks" in Etalon. vhosts on the same physical machine connect to the
physical NIC using a virtualized switch (macvlan via docker), using ```tc``` to
limit vhost link bandwidth (similar to a host to ToR link in a real datacenter).

A separate physical machine emulates the reconfigurable switch in Etalon. It
does this using a software switch (Click). Within the software switch, we
emulate ToR VOQs, a circuit switch, and a packet switch. All parameters for
these elements are adjustable (e.g., VOQ length, circuit switch reconfiguration
penalty, link bandwidths/delays, etc.).

Each "rack" is connected to the software switch via a physical switch in the
testbed (e.g., a 40Gbps switch in CloudLab's Apt cluster). We also assume a
second (control) network connects each "rack" and the software switch for
convenience.

Finally, to allow our system to scale beyond a single 40Gbps link, we use time
dilation (```libVT```) to emulate many 40Gbps links simultaneously.

See our paper for more information.


## Instructions

Cluster Setup
-------------

Coming soon...


Running An Experiment
---------------------

After finishing the initial setup, creating an experiment, running it, and
collecting the results are all handled by our experiment framework, run on the
software switch machine.

The basic workflow for running an experiment on Etalon is:

1. On the software switch machine, launch ```/etalon/bin/click_startup.sh```.

2. In another window on the software switch machine, launch an experiment file
script (e.g., ```cd /etalon/experiments/buffers; ./buffers.py```).

The experiment file script will build a docker image (on the software switch
machine), push the image to all of the "rack" machines, launch a "rack" of
vhosts, run the experiment (e.g., running 16 flows for 40 seconds), and then
collect the results (e.g., logs), and tar them. These logs can be processed by
graphing scripts which we provide (see [Graphing the
Results](#graphing-the-results) below).

See ```experiments/buffers/buffers.py``` and
```experiments/buffers/buffer_common.py``` for an example of an experiment file
script. Generally, an experiment file script will call
```initializeExperiment(image)```, loop through some switch configurations
(e.g., changing ToR VOQ buffer sizes) while running some benchmark on each
configuration (e.g., flowgrind or dfsio), and then call
```finishExperiment()```. The resulting tarball will always be in the directory
where the script was launched from, and be named based on the timestamp at
launch. ```finishExperiment()``` prints out this timestamp to make it easy to
distinguish multiple resulting tar files.


Graphing the Results
--------------------

After running an experiment, there is a tar file with the results on the
software switch machine. We assume that most users will want the option to
archive the raw result data on their local machine, so we assume graphing
scripts will be run on the local machine, not on the software switch machine.

To graph results:

1. Copy the results tar file to your local machine. The file ```sxp.py``` in
```etalon/bin``` can simplify this. Assuming your ```etalon/etc/handles``` file
is setup correctly, ```sxp.py``` will try to ```scp``` a results file from the
folders ```experiments/buffers/```, ```experiments/adu```, or
```experiment/hdfs``` on the software switch machine to the local machine, given
some timestamp. e.g., ```./sxp.py 1519494787``` will look in those directories
for a tar file with 1519494787 in its name.

2. Untar the results file, into a new results folder (macOS' built in unarchiver
will do this automatically, on Linux you'll need to create a folder first and
put the tar in that folder and unarchive).

3. Call a graphing script with the results folder as an argument , e.g., ```cd
experiments/buffers/ ; ./buffer_graphs.py ../../bin/1519494787-buffers```. This
expects a folder named ```graphs``` to exist in the current folder.

After running this you should have some graphs in a subfolder called
```graphs```. We include all graphing scripts used to create the graphs in our
paper. If you wish to create other graphs, ```experiments/parse_logs.py``` will
likely be useful.


## Directory Overview

Read the README.md in each subfolder for more information.

- ```bin```: various runables (e.g., performance tuning scripts, click script
  generation, installation scripts).

- ```click-etalon```: our software switch emulation; a modified version of Click.

- ```etc```: various configurations scripts.

- ```experiments```: experiment framework and various example experiment file
  scripts.

- ```flowgrind-etalon```: our flow generator; a modified version of flowgrind.

- ```libADU```: an interposition library that tells the software switch how much
  data is waiting in the endhost stack.

- ```libVT```: a virtual time interposition library ("time dilation") that we
  use to scale the number of links.

- ```reHDFS```: our modified HDFS write replica placement algorithm for
  reconfigurable datacenter networks.

- ```reTCP```: our TCP congestion control variant for reconfigurable datacenter
  networks.

- ```rpycd```: a daemon that runs on each "rack" that allows the software switch
  machine to setup experiments on each "rack".

- ```vhost```: code and supporting files for building the vhost image (docker
  image).


## Questions, Comments, Feedback

We'd be happy to hear if you're using our emulator in your research. Let us know
if you have any questions, comments, or feedback by sending an email to
```mukerjee at cs cmu edu```.
