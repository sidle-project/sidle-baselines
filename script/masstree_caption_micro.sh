#!/bin/bash
warmup=0
runtime=60
builddir=$1
output_dir=$2
caption_dir="../benchmark/caption/"
cur_dir=$(pwd)
bgn=0
fgn=28
size=30000000
with_insert_size=30000000
limit_cpu_cache=0
cpu_list=0-47
cat_prefix="sudo -S -k rdtset -t 'l2=0x1;l3=0x1;cpu=${cpu_list}' -c ${cpu_list}"
echo "cat_prefix=$cat_prefix"
zipfian_theta=0.99
hot_data_ratio=5
hot_query_ratio=90
hot_data_start=5

mutiple=4

run_rdtset() {
    local cpu_range=$cpu_list  
    if [ "$limit_cpu_cache" -eq 1 ]; then
        sudo -S -k rdtset -t "l2=0x1;l3=0x1;cpu=${cpu_range}" -c "${cpu_range}" "$@"
    else
        "$@"
    fi
}

source ./utils/env_setup.sh

rm -rf $builddir
mkdir -p $builddir
cd $builddir
builddir=$(pwd)
echo "builddir=$builddir"
cmake -DCMAKE_BUILD_TYPE=release ..
make -j 32
cd $cur_dir
mkdir -p $output_dir
cd $output_dir
output_dir=$(pwd)
echo "output_dir=$output_dir"
cd $cur_dir
touch $caption_dir/example_input/sidle_micro.sh
chmod +x $caption_dir/example_input/sidle_micro.sh

for target in masstree
do
    for cxl_percentage in 90
    do
        if [ ! -f $output_dir/${target}-skewed-update-heavy-${fgn}-${size}-${cxl_percentage}.txt ];then
            echo "------[Overall] skewed_partition update-heavy origin, target=$target, fgn=$fgn, bgn=$bgn, cxl_percentage=$cxl_percentage"
            echo "$builddir/skewed_partition -a 0.5 -b 0.5 --target $target --runtime $runtime --warmup $warmup --fg $fgn --bg $bgn --table-size $size"
            echo "numactl --interleave=0,2 $builddir/skewed_partition \
                -a 0.5 \
                -b 0.5 \
                --target $target \
                --runtime $runtime \
                --warmup $warmup \
                --fg $fgn \
                --bg $bgn \
                --table-size $size --cxl-percentage $cxl_percentage \
                --hot-data-ratio $hot_data_ratio --hot-query-ratio $hot_query_ratio \
                --hot-data-start $hot_data_start > $output_dir/${target}-skewed-update-heavy-${fgn}-${size}-${cxl_percentage}.txt" > $caption_dir/example_input/sidle_micro.sh
            sudo python3 $caption_dir/caption.py -s $caption_dir/example_input/sidle_micro.sh -r $mutiple
        fi

        if [ ! -f $output_dir/${target}-skewed-read-mostly-${fgn}-${size}-${cxl_percentage}.txt ];then
            echo "------[Overall] skewed_partition read-mostly origin, target=$target, fgn=$fgn, bgn=$bgn, cxl_percentage=$cxl_percentage"
            echo "numactl --interleave=0,2 $builddir/skewed_partition \
                -a 0.9 \
                -b 0.1 \
                --target $target \
                --runtime $runtime \
                --warmup $warmup \
                --fg $fgn \
                --bg $bgn \
                --table-size $size --cxl-percentage $cxl_percentage \
                --hot-data-ratio $hot_data_ratio --hot-query-ratio $hot_query_ratio \
                --hot-data-start $hot_data_start > $output_dir/${target}-skewed-read-mostly-${fgn}-${size}-${cxl_percentage}.txt" > $caption_dir/example_input/sidle_micro.sh
            sudo python3 $caption_dir/caption.py -s $caption_dir/example_input/sidle_micro.sh -r $mutiple
        fi

        if [ ! -f $output_dir/${target}-skewed-read-only-${fgn}-${size}-${cxl_percentage}.txt ];then
            echo "------[Overall] skewed_partition read-only origin, target=$target, fgn=$fgn, bgn=$bgn, cxl_percentage=$cxl_percentage"
            echo "numactl --interleave=0,2 $builddir/skewed_partition \
                -a 1 \
                --target $target \
                --runtime $runtime \
                --warmup $warmup \
                --fg $fgn \
                --bg $bgn \
                --table-size $size --cxl-percentage $cxl_percentage \
                --hot-data-ratio $hot_data_ratio --hot-query-ratio $hot_query_ratio \
                --hot-data-start $hot_data_start > $output_dir/${target}-skewed-read-only-${fgn}-${size}-${cxl_percentage}.txt" > $caption_dir/example_input/sidle_micro.sh
            sudo python3 $caption_dir/caption.py -s $caption_dir/example_input/sidle_micro.sh -r $mutiple
        fi 

        if [ ! -f $output_dir/${target}-skewed-with-insert-${fgn}-${size}-${cxl_percentage}.txt ];then
            echo "------[Overall] skewed_partition with insert origin, target=$target, fgn=$fgn, bgn=$bgn, cxl_percentage=$cxl_percentage"
            echo "numactl --interleave=0,2 $builddir/skewed_partition_dynamic \
                -a 0.95 \
                -b 0.05 \
                --target $target \
                --runtime 30 \
                --warmup $warmup \
                --fg $fgn \
                --bg $bgn \
                --table-size $with_insert_size --cxl-percentage $cxl_percentage \
                --hot-data-ratio $hot_data_ratio --hot-query-ratio $hot_query_ratio \
                --hot-data-start $hot_data_start > $output_dir/${target}-skewed-with-insert-${fgn}-${size}-${cxl_percentage}.txt" > $caption_dir/example_input/sidle_micro.sh
            sudo python3 $caption_dir/caption.py -s $caption_dir/example_input/sidle_micro.sh -r $mutiple
        fi
    done
done


