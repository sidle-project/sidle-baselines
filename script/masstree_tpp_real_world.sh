#!/bin/bash
warmup=0
runtime=60
builddir=$1
output_dir=$2
cur_dir=$(pwd)
bgn=0
fgn=28
load_data_dir=/data/user/lib/datasets/alibaba/load_data_new
trace_dir=/data/user/lib/datasets/alibaba/new_candidate
local_memory_list=(200 200)
art_local_memory_list=(1000 1050)
cxl_percentage=80
reserve_memory_percentage=1

source ./utils/env_setup.sh

calculate_reserve_memory() {
    source ./utils/env_setup.sh
    sudo sh -c "sudo echo 1 > /sys/kernel/mm/numa/demotion_enabled"
    sudo sh -c "sudo echo 3 > /proc/sys/kernel/numa_balancing"
    local local_memory=$1
    total_memory_mb=$(numactl --hardware | grep "node 0 size" | awk '{print $4}')
    unused_memory_mb=$(numactl --hardware | grep "node 0 free" | awk '{print $4}')
    reserve_memory=$((unused_memory_mb-local_memory))
    echo "reserve_memory=$reserve_memory"
    echo "total_memory_mb=$total_memory_mb"
    reserve_memory_percentage=$(echo "scale=5; $reserve_memory / $total_memory_mb * 10000" | bc)
    reserve_memory_percentage=$(printf "%.0f" "$reserve_memory_percentage")
    sudo sh -c "sudo echo $reserve_memory_percentage > /proc/sys/vm/demote_scale_factor"
}

rm -rf $builddir
mkdir -p $builddir
cd $builddir
cmake -DCMAKE_BUILD_TYPE=release ..
make -j 32
cd $cur_dir

for target in masstree art
do
    cur_output_dir=${output_dir}_${target}
    mkdir -p $cur_output_dir
    if [ "$target" == "art" ]; then
        runtime=30
    fi
    # 7
    echo "------[Overall] real_world_trace, target=$target, fgn=$fgn, bgn=$bgn, cxl_percentage=$cxl_percentage, device_id=7"
    local_memory=${local_memory_list[0]}
    if [ $target == "art" ]; then
        local_memory=${art_local_memory_list[0]}
    fi
    calculate_reserve_memory ${local_memory}
    echo "reserve_memory_percentage=$reserve_memory_percentage"
    numactl --cpubind=0 --membind=0,2 ./$builddir/real_world_trace --target $target --runtime $runtime --warmup $warmup --fg $fgn --bg $bgn --file $load_data_dir/7.in --second-file $trace_dir/input_7_0.csv --cxl-percentage $cxl_percentage > $cur_output_dir/7_baseline_${cxl_percentage}.txt

    # 40
    echo "------[Overall] real_world_trace, target=$target, fgn=$fgn, bgn=$bgn, cxl_percentage=$cxl_percentage, device_id=40"
    local_memory=${local_memory_list[1]}
    if [ $target == "art" ]; then
        local_memory=${art_local_memory_list[1]}
    fi
    calculate_reserve_memory ${local_memory}
    echo "reserve_memory_percentage=$reserve_memory_percentage"
    numactl --cpubind=0 --membind=0,2 ./$builddir/real_world_trace --target $target --runtime $runtime --warmup $warmup --fg $fgn --bg $bgn --file $load_data_dir/40.in --second-file $trace_dir/input_40_0.csv --cxl-percentage $cxl_percentage > $cur_output_dir/40_baseline_${cxl_percentage}.txt
done
