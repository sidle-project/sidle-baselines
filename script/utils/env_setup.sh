#!/bin/bash

# disable hugepage
sudo sh -c "echo 0 > /proc/sys/vm/nr_hugepages"

# set cpu frequency to performance mode
sudo cpupower frequency-set --governor performance

# flush cache
sudo sh -c 'sync; echo 3 > /proc/sys/vm/drop_caches'

# enable cxl device 
sudo chmod 777 /dev/dax0.0

# configure cxl device as numa node
sudo daxctl reconfigure-device -f --mode=system-ram dax0.0