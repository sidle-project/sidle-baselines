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
target=masstree

local_memory_list=(320 700 600)

# env setup

# set cpu frequency to performance mode
sudo cpupower frequency-set --governor performance

# flush cache
sudo sh -c 'sync; echo 3 > /proc/sys/vm/drop_caches'

# enable cxl device 
sudo chmod 777 /dev/dax0.0

# configure cxl device as numa node
sudo daxctl reconfigure-device -f --mode=system-ram dax0.0

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
        # local_memory_list=(480 990 480)
        local_memory_list=(440 750 440)
    fi
    for cxl_percentage in 80
    do
        if [ ! -f $output_dir/${target}-update-heavy-${fgn}-${size}-${cxl_percentage}.txt ];then
            echo "------[Overall] ycsb update-heavy origin, target=$target, fgn=$fgn, bgn=$bgn, cxl_percentage=$cxl_percentage"
            echo "$builddir/rw_ycsb_origin -a 0.5 -b 0.5 --target $target --runtime $runtime --warmup $warmup --fg $fgn --bg $bgn --table-size $size"
            echo "memory: sudo ./memtis_setup.sh -B masstree -R static -D ${local_memory_list[0]}MB"
            sudo ./memtis_setup.sh -B masstree -R static -D ${local_memory_list[0]}MB --cxl -V memtis-cxl -C ./rw_ycsb_origin \
                -a 0.5 \
                -b 0.5 \
                --target $target \
                --runtime $runtime \
                --warmup $warmup \
                --fg $fgn \
                --bg $bgn \
                --table-size $size > $output_dir/${target}-update-heavy-${fgn}-${size}-${cxl_percentage}.txt
        fi

        if [ ! -f $output_dir/${target}-read-mostly-${fgn}-${size}-${cxl_percentage}.txt ];then
            echo "------[Overall] ycsb read-mostly origin, target=$target, fgn=$fgn, bgn=$bgn, cxl_percentage=$cxl_percentage"
            echo "memory: sudo ./memtis_setup.sh -B masstree -R static -D ${local_memory_list[0]}MB"
            sudo ./memtis_setup.sh -B masstree -R static -D ${local_memory_list[0]}MB --cxl -V memtis-cxl -C ./rw_ycsb_origin \
                -a 0.9 \
                -b 0.1 \
                --target $target \
                --runtime $runtime \
                --warmup $warmup \
                --fg $fgn \
                --bg $bgn \
                --table-size $size > $output_dir/${target}-read-mostly-${fgn}-${size}-${cxl_percentage}.txt
        fi

        if [ ! -f $output_dir/${target}-read-only-${fgn}-${size}-${cxl_percentage}.txt ];then
            echo "------[Overall] ycsb read-only origin, target=$target, fgn=$fgn, bgn=$bgn, cxl_percentage=$cxl_percentage"
            echo "memory: sudo ./memtis_setup.sh -B masstree -R static -D ${local_memory_list[0]}MB"
            sudo ./memtis_setup.sh -B masstree -R static -D ${local_memory_list[0]}MB --cxl -V memtis-cxl -C ./rw_ycsb_origin \
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
            echo "memory: sudo ./memtis_setup.sh -B masstree -R static -D ${local_memory_list[1]}MB"
            sudo ./memtis_setup.sh -B masstree -R static -D ${local_memory_list[1]}MB --cxl -V memtis-cxl -C ./read_latest_origin \
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
                echo "memory: sudo ./memtis_setup.sh -B masstree -R static -D ${local_memory_list[2]}MB"
                sudo ./memtis_setup.sh -B masstree -R static -D ${local_memory_list[2]}MB --cxl -V memtis-cxl -C ./short_range_origin \
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
            echo "memory: sudo ./memtis_setup.sh -B masstree -R static -D ${local_memory_list[0]}MB"
            sudo ./memtis_setup.sh -B masstree -R static -D ${local_memory_list[0]}MB --cxl -V memtis-cxl -C ./read_modify_write_origin \
                --target $target \
                --runtime $runtime \
                --warmup $warmup \
                --fg $fgn \
                --bg $bgn \
                --table-size $size --cxl-percentage $cxl_percentage > $output_dir/${target}-read-modify-write-${fgn}-${size}-${cxl_percentage}.txt
        fi
    done
done


