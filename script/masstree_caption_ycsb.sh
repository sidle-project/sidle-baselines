#!/bin/bash
warmup=0
runtime=60
builddir=$1
output_dir=$2
caption_dir="../benchmark/caption/"
cur_dir=$(pwd)
bgn=0
fgn=28
size=20000000
limit_cpu_cache=0
cpu_list=0-47
cat_prefix="sudo -S -k rdtset -t 'l2=0x1;l3=0x1;cpu=${cpu_list}' -c ${cpu_list}"
echo "cat_prefix=$cat_prefix"
zipfian_theta=0.99
mutiple=2.3

source ./utils/env_setup.sh

run_rdtset() {
    local cpu_range=$cpu_list  
    if [ "$limit_cpu_cache" -eq 1 ]; then
        sudo -S -k rdtset -t "l2=0x1;l3=0x1;cpu=${cpu_range}" -c "${cpu_range}" "$@"
    else
        "$@"
    fi
}



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
touch $caption_dir/example_input/sidle_ycsb.sh
chmod +x $caption_dir/example_input/sidle_ycsb.sh

# for target in masstree
for target in art
do
    for cxl_percentage in 80
    do
        if [ ! -f $output_dir/${target}-update-heavy-${fgn}-${size}-${cxl_percentage}.txt ];then
            echo "------[Overall] ycsb update-heavy origin, target=$target, fgn=$fgn, bgn=$bgn, cxl_percentage=$cxl_percentage"
            echo "$builddir/rw_ycsb_origin -a 0.5 -b 0.5 --target $target --runtime $runtime --warmup $warmup --fg $fgn --bg $bgn --table-size $size"

            echo "numactl --interleave=0,2 $builddir/rw_ycsb_origin \
                -a 0.5 \
                -b 0.5 \
                --target $target \
                --runtime $runtime \
                --warmup $warmup \
                --fg $fgn \
                --bg $bgn \
                --table-size $size --cxl-percentage $cxl_percentage > $output_dir/${target}-update-heavy-${fgn}-${size}-${cxl_percentage}.txt" > $caption_dir/example_input/sidle_ycsb.sh
            sudo python3 $caption_dir/caption.py -s $caption_dir/example_input/sidle_ycsb.sh -r $mutiple
        fi

        if [ ! -f $output_dir/${target}-read-mostly-${fgn}-${size}-${cxl_percentage}.txt ];then
            echo "------[Overall] ycsb read-mostly origin, target=$target, fgn=$fgn, bgn=$bgn, cxl_percentage=$cxl_percentage"
            echo "numactl --interleave=0,2 $builddir/rw_ycsb_origin \
                -a 0.9 \
                -b 0.1 \
                --target $target \
                --runtime $runtime \
                --warmup $warmup \
                --fg $fgn \
                --bg $bgn \
                --table-size $size --cxl-percentage $cxl_percentage > $output_dir/${target}-read-mostly-${fgn}-${size}-${cxl_percentage}.txt" > $caption_dir/example_input/sidle_ycsb.sh
            sudo python3 $caption_dir/caption.py -s $caption_dir/example_input/sidle_ycsb.sh -r $mutiple
        fi

        if [ ! -f $output_dir/${target}-read-only-${fgn}-${size}-${cxl_percentage}.txt ];then
            echo "------[Overall] ycsb read-only origin, target=$target, fgn=$fgn, bgn=$bgn, cxl_percentage=$cxl_percentage"
            echo "numactl --interleave=0,2 $builddir/rw_ycsb_origin \
                -a 1 \
                --target $target \
                --runtime $runtime \
                --warmup $warmup \
                --fg $fgn \
                --bg $bgn \
                --table-size $size --cxl-percentage $cxl_percentage > $output_dir/${target}-read-only-${fgn}-${size}-${cxl_percentage}.txt" > $caption_dir/example_input/sidle_ycsb.sh
            sudo python3 $caption_dir/caption.py -s $caption_dir/example_input/sidle_ycsb.sh -r $mutiple
        fi

        if [ ! -f $output_dir/${target}-read-latest-${fgn}-${size}-${cxl_percentage}.txt ];then
            echo "------[Overall] ycsb read-latest origin, target=$target, fgn=$fgn, bgn=$bgn, cxl_percentage=$cxl_percentage"
            echo "numactl --interleave=0,2 $builddir/read_latest_origin \
                --target $target \
                --runtime 30 \
                --warmup $warmup \
                --fg $fgn \
                --bg $bgn \
                --table-size $size --cxl-percentage $cxl_percentage > $output_dir/${target}-read-latest-${fgn}-${size}-${cxl_percentage}.txt" > $caption_dir/example_input/sidle_ycsb.sh
            sudo python3 $caption_dir/caption.py -s $caption_dir/example_input/sidle_ycsb.sh -r $mutiple
        fi
        
        if [ "$target" != "art" ]; then
            if [ ! -f $output_dir/${target}-short-ranges-${fgn}-${size}-${cxl_percentage}.txt ];then
                echo "------[Overall] ycsb short-ranges origin, target=$target, fgn=$fgn, bgn=$bgn, cxl_percentage=$cxl_percentage"
                echo "numactl --interleave=0,2 $builddir/short_range_origin \
                    --target $target \
                    --runtime 30 \
                    --warmup $warmup \
                    --fg $fgn \
                    --bg $bgn \
                    --table-size $size --cxl-percentage $cxl_percentage > $output_dir/${target}-short-ranges-${fgn}-${size}-${cxl_percentage}.txt" > $caption_dir/example_input/sidle_ycsb.sh
                sudo python3 $caption_dir/caption.py -s $caption_dir/example_input/sidle_ycsb.sh -r $mutiple
            fi
        fi

        if [ ! -f $output_dir/${target}-read-modify-write-${fgn}-${size}-${cxl_percentage}.txt ];then
            echo "------[Overall] ycsb read-modify-write origin, target=$target, fgn=$fgn, bgn=$bgn, cxl_percentage=$cxl_percentage"
            echo "numactl --interleave=0,2 $builddir/read_modify_write_origin \
                --target $target \
                --runtime $runtime \
                --warmup $warmup \
                --fg $fgn \
                --bg $bgn \
                --table-size $size --cxl-percentage $cxl_percentage > $output_dir/${target}-read-modify-write-${fgn}-${size}-${cxl_percentage}.txt" > $caption_dir/example_input/sidle_ycsb.sh
            sudo python3 $caption_dir/caption.py -s $caption_dir/example_input/sidle_ycsb.sh -r $mutiple
        fi
    done
done


