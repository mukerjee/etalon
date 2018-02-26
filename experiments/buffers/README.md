# experiments/buffers

Runs buffer experiments and graphs them (section 4 of our paper).

- ```buffer_common.py```: Common configuration information across buffer
  experiments.

- ```buffer_graphs.py```: Generates graphs for buffer experiments, specifically
  figures 6, 7, 8, and 9 in our paper. Expects a directory of output data from
  ```buffers.py``` as an argument.

- ```buffers.py```: Experiment file for buffer experiments.

- ```delay_graphs.py```: Generates delay graphs for buffer experiments,
  specifically figure 10 in our paper. Expects a directory of output data from
  ```delay_sensitivity.py``` as an argument.

- ```delay_sensitivity.py```: Experiment file for delay sensitivity analysis for
  buffer experiments.

- ```sequence_graphs.py```: Generates sequence graphs for buffer experiments,
  specifically figure 5 in our paper. Expects a directory of output data from
  ```buffers.py``` as an argument.

- ```validation.py```: Experiment file for validating Elaton.

- ```validation_results.py```: Generates validation table data for buffer
  experiments, seen in Table 1 in our paper. Expects a directory of output data
  from ```validation.py``` as an argument.
