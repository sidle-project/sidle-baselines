#!/bin/bash
warmup=0
runtime=60
builddir=$1
output_dir=$2
cur_dir=$(pwd)
bgn=0
fgn=28
size=20000000
limit_cpu_cache=0
cpu_list=0-47
cat_prefix="sudo -S -k rdtset -t 'l2=0x1;l3=0x1;cpu=${cpu_list}' -c ${cpu_list}"
echo "cat_prefix=$cat_prefix"
zipfian_theta=0.99

local_memory_list=(420 854 730)
reserve_memory_percentage=1

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
mkdir -p $output_dir

# for target in masstree
for target in art
do
    if [ "$target" == "art" ]; then
        # local_memory_list=(110 263 110)
        # local_memory_list=(440 750 440)
        local_memory_list=(180 670 240)
    fi
    for cxl_percentage in 80
    do
        if [ ! -f $output_dir/${target}-update-heavy-${fgn}-${size}-${cxl_percentage}.txt ];then
            echo "------[Overall] ycsb update-heavy origin, target=$target, fgn=$fgn, bgn=$bgn, cxl_percentage=$cxl_percentage"
            echo "$builddir/rw_ycsb_origin -a 0.5 -b 0.5 --target $target --runtime $runtime --warmup $warmup --fg $fgn --bg $bgn --table-size $size"
            calculate_reserve_memory ${local_memory_list[0]}
            echo "reserve_memory_percentage=$reserve_memory_percentage"
            numactl --cpubind=0 --membind=0,2 ./$builddir/rw_ycsb_origin \
                -a 0.5 \
                -b 0.5 \
                --target $target \
                --runtime $runtime \
                --warmup $warmup \
                --fg $fgn \
                --bg $bgn \
                --table-size $size --cxl-percentage $cxl_percentage > $output_dir/${target}-update-heavy-${fgn}-${size}-${cxl_percentage}.txt
        fi

        if [ ! -f $output_dir/${target}-read-mostly-${fgn}-${size}-${cxl_percentage}.txt ];then
            echo "------[Overall] ycsb read-mostly origin, target=$target, fgn=$fgn, bgn=$bgn, cxl_percentage=$cxl_percentage"
            calculate_reserve_memory ${local_memory_list[0]}
            echo "reserve_memory_percentage=$reserve_memory_percentage"
            numactl --cpubind=0 --membind=0,2 ./$builddir/rw_ycsb_origin \
                -a 0.9 \
                -b 0.1 \
                --target $target \
                --runtime $runtime \
                --warmup $warmup \
                --fg $fgn \
                --bg $bgn \
                --table-size $size --cxl-percentage $cxl_percentage > $output_dir/${target}-read-mostly-${fgn}-${size}-${cxl_percentage}.txt
        fi

        if [ ! -f $output_dir/${target}-read-only-${fgn}-${size}-${cxl_percentage}.txt ];then
            echo "------[Overall] ycsb read-only origin, target=$target, fgn=$fgn, bgn=$bgn, cxl_percentage=$cxl_percentage"
            calculate_reserve_memory ${local_memory_list[0]}
            echo "reserve_memory_percentage=$reserve_memory_percentage"
            numactl --cpubind=0 --membind=0,2 ./$builddir/rw_ycsb_origin \
                -a 1 \
                --target $target \
                --runtime $runtime \
                --warmup $warmup \
                --fg $fgn \
                --bg $bgn \
                --table-size $size --cxl-percentage $cxl_percentage > $output_dir/${target}-read-only-${fgn}-${size}-${cxl_percentage}.txt
        fi

        if [ ! -f $output_dir/${target}-read-latest-${fgn}-${size}-${cxl_percentage}.txt ];then
            echo "------[Overall] ycsb read-latest origin, target=$target, fgn=$fgn, bgn=$bgn, cxl_percentage=$cxl_percentage"
            calculate_reserve_memory ${local_memory_list[1]}
            echo "reserve_memory_percentage=$reserve_memory_percentage"
            numactl --cpubind=0 --membind=0,2 ./$builddir/read_latest_origin \
                --target $target \
                --runtime 30 \
                --warmup $warmup \
                --fg $fgn \
                --bg $bgn \
                --table-size $size --cxl-percentage $cxl_percentage > $output_dir/${target}-read-latest-${fgn}-${size}-${cxl_percentage}.txt
        fi

        if [ "$target" != "art" ]; then
            if [ ! -f $output_dir/${target}-short-ranges-${fgn}-${size}-${cxl_percentage}.txt ];then
                echo "------[Overall] ycsb short-ranges origin, target=$target, fgn=$fgn, bgn=$bgn, cxl_percentage=$cxl_percentage"
                calculate_reserve_memory ${local_memory_list[2]}
                echo "reserve_memory_percentage=$reserve_memory_percentage"
                numactl --cpubind=0 --membind=0,2 ./$builddir/short_range_origin \
                    --target $target \
                    --runtime 30 \
                    --warmup 5 \
                    --fg $fgn \
                    --bg $bgn \
                    --table-size $size --cxl-percentage $cxl_percentage > $output_dir/${target}-short-ranges-${fgn}-${size}-${cxl_percentage}.txt
            fi
        fi

        if [ ! -f $output_dir/${target}-read-modify-write-${fgn}-${size}-${cxl_percentage}.txt ];then
            echo "------[Overall] ycsb read-modify-write origin, target=$target, fgn=$fgn, bgn=$bgn, cxl_percentage=$cxl_percentage"
            calculate_reserve_memory ${local_memory_list[0]}
            echo "reserve_memory_percentage=$reserve_memory_percentage"
            numactl --cpubind=0 --membind=0,2 ./$builddir/read_modify_write_origin \
                --target $target \
                --runtime $runtime \
                --warmup $warmup \
                --fg $fgn \
                --bg $bgn \
                --table-size $size --cxl-percentage $cxl_percentage > $output_dir/${target}-read-modify-write-${fgn}-${size}-${cxl_percentage}.txt
        fi
    done
done


