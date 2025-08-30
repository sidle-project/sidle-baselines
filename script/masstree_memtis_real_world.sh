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
art_local_memory_list=(4000 4050)

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
# mkdir -p $output_dir

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
  local_memory=${local_memory_list[0]}
  if [ $target == "art" ]; then
    local_memory=${art_local_memory_list[0]}
  fi
  echo "memory: sudo ./memtis_setup.sh -B masstree -R static -D ${local_memory}MB"
  sudo ./memtis_setup.sh -B masstree -R static -D ${local_memory}MB --cxl -V memtis-cxl -C ./$builddir/real_world_trace --target $target --runtime $runtime --warmup $warmup --fg $fgn --bg $bgn --file $load_data_dir/7.in --second-file $trace_dir/input_7_0.csv --cxl-percentage $cxl_percentage > $cur_output_dir/7_baseline_${cxl_percentage}.txt


  # 40
  echo "------[Overall] real_world_trace, target=$target, fgn=$fgn, bgn=$bgn, cxl_percentage=$cxl_percentage, device_id=40"
  local_memory=${local_memory_list[1]}
  if [ $target == "art" ]; then
    local_memory=${art_local_memory_list[1]}
  fi
  echo "memory: sudo ./memtis_setup.sh -B masstree -R static -D ${local_memory}MB"
  sudo ./memtis_setup.sh -B masstree -R static -D ${local_memory}MB --cxl -V memtis-cxl -C ./$builddir/real_world_trace --target $target --runtime $runtime --warmup $warmup --fg $fgn --bg $bgn --file $load_data_dir/40.in --second-file $trace_dir/input_40_0.csv --cxl-percentage $cxl_percentage > $cur_output_dir/40_baseline_${cxl_percentage}.txt
done
