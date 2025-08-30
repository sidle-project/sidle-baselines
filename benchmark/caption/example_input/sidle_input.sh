#!/bin/bash
# node 0 is for dram, node 2 is for CXL
numactl --interleave=0,2 ../../build/rw_ycsb_origin -a 0.9 -b 0.1 --target masstree --runtime 30 --warmup 0 --fg 28 --bg 0 --table-size 20000000 > spd.log