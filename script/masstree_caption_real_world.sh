warmup=0
runtime=60
builddir=$1
output_dir=$2
caption_dir="../benchmark/caption/"
cur_dir=$(pwd)
bgn=0
fgn=28
load_data_dir=/data/user/lib/datasets/alibaba/load_data_new
trace_dir=/data/user/lib/datasets/alibaba/new_candidate
mutiple=3

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
touch $caption_dir/example_input/sidle_real_world.sh
chmod +x $caption_dir/example_input/sidle_real_world.sh

cxl_percentage=80

for target in masstree art
do
    cur_output_dir=${output_dir}_${target}
    mkdir -p $cur_output_dir
    if [ "$target" == "art" ]; then
        runtime=30
    fi
    # 7
    echo "------[Overall] real_world_trace, target=$target, fgn=$fgn, bgn=$bgn, cxl_percentage=$cxl_percentage, device_id=7"
    echo "numactl --interleave=0,2 /$builddir/real_world_trace \
        --target $target --runtime $runtime --warmup $warmup --fg $fgn \
        --bg $bgn --file $load_data_dir/7.in --second-file $trace_dir/input_7_0.csv \
        --cxl-percentage $cxl_percentage > $cur_output_dir/7_baseline_${cxl_percentage}.txt" > $caption_dir/example_input/sidle_real_world.sh
    sudo python3 $caption_dir/caption.py -s $caption_dir/example_input/sidle_real_world.sh -r $mutiple

    # 40
    echo "------[Overall] real_world_trace, target=$target, fgn=$fgn, bgn=$bgn, cxl_percentage=$cxl_percentage, device_id=40"
    echo "numactl --interleave=0,2 /$builddir/real_world_trace \
        --target $target --runtime $runtime --warmup $warmup --fg $fgn \
        --bg $bgn --file $load_data_dir/40.in --second-file $trace_dir/input_40_0.csv \
        --cxl-percentage $cxl_percentage > $cur_output_dir/40_baseline_${cxl_percentage}.txt" > $caption_dir/example_input/sidle_real_world.sh
    sudo python3 $caption_dir/caption.py -s $caption_dir/example_input/sidle_real_world.sh -r $mutiple
done