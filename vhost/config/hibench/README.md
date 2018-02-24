# vhost/config/hibench

Configuration files for hibench hadoop benchmarking (DFSIOE). The only important
change here is that we've modified DFSIOE to write 64 files that are each
400MB. We choose 64 files as YARN creates 64 mappers on our cluster (one per
physical core). Thus we can complete this workload in one round of map.