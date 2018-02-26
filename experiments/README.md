# experiments
=============

Framework for running experiments using Etalon, as well as some example
experiments and graphing scripts from our paper:

Framework
---------

- ```click_common.py```: Functions for modifying / retrieving info to /from the
  software switch.

- ```common.py```: Experiment framework. Provides functions for starting
  experiments (e.g., launching vhosts on each physical machines), running a
  benchmark app (e.g., flowgrind, dfsioe), and collecting the results (e.g.,
  logs).

- ```parse_logs.py```: Various log processing functions used in our graphing
  scripts.

Example experiments
-------------------

- ```buffers```: Buffer experiments (section 4 of our paper).

- ```adu```: ADU experiments (section 5 of our paper).

- ```hdfs```: HDFS experiments (section 6 of our paper).


See subfolder README.md for more details.
