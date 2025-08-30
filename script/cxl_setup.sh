#!/bin/bash

source ./utils/env_check.sh

# store the total memory size of each node and the free memory size of each node
declare -A node_sizes
declare -A node_free
cxl_node=-1
host_node=-1

# (1) check whether the cxl node exist, if it exists, the node will have no cpu
check_cxl_node() 
{
  node_count=$(numactl --hardware | grep 'available:' | awk '{print $2}')
  for node in $(seq 0 $((node_count-1))); do
    cpus=$(numactl --hardware | grep "node ${node} cpus" | awk '{print $4}')
    echo "check_cxl_node: ${node} $cpus"
    if [[ -z $cpus ]]; then
        cxl_node=$node
        break
    fi
  done

  if [[ $cxl_node -eq -1 ]]; then
    echo "no cxl node exists, please check your environment"
    exit 1
  else
    echo "the cxl node is ${cxl_node}"
  fi
}

# (2) get the nodes' memory info
extract_memory_info() 
{
  node_count=$(numactl --hardware | grep 'available:' | awk '{print $2}')
  
  for node in $(seq 0 $((node_count - 1))); do
    size=$(numactl --hardware | grep "node ${node} size" | awk '{print $4}')
    free=$(numactl --hardware | grep "node ${node} free" | awk '{print $4}')
    node_sizes[$node]=$size
    node_free[$node]=$free
  done
}

# (3) estimate the effect of numa
estimate_numa_effect() 
{
    if [[ $node_count -eq 2 ]]; then
        echo "no extra numa node exists"
        return
    fi
    
    # find the host node which has the closest free memory size to the cxl node
    local closest_node=-1
    local closest_memory_diff=10000000000
    for node in "${!node_free[@]}"
    do
        if [[ $node -eq $cxl_node ]]; then
            continue
        fi
        diff=$((node_free[$node] - node_free[$cxl_node]))
        if [[ $diff -lt 0 ]]; then
            diff=$((0 - diff))
        fi
        if [[ $closest_node -eq -1 ]]; then
            closest_memory_diff=$diff
            closest_node=$node
            continue
        fi
        echo "diff: $diff, closest_memory_diff: $closest_memory_diff"
        if [[ $diff -lt $closest_memory_diff ]]; then
            closest_memory_diff=$diff
            closest_node=$node
            echo "get here, choose node"

        fi
    done

    host_node=$closest_node
    echo "host_node: $host_node"
    # disable memory for all the numa nodes except the host node and the cxl node
    for node in "${!node_free[@]}"; do
        if [[ $node -eq $host_node || $node -eq $cxl_node ]]; then
            continue
        fi
        echo "disable_nodex_mem $node"
        disable_nodex_mem $node
    done
}

# (4) setup the cxl environment
cxl_env() {
    # basic env check
    check_base_conf
    # check and record the cxl info
    check_cxl_node
    extract_memory_info
    estimate_numa_effect
}

# (5) use the "numa" (actually cxl) policy is to allocate memory for the target process
run_on_cxl() 
{
    local MEM_SHOULD_RESERVE=0
    # $2: exp type (100, 50, 0, "Interleave")
    # "100" -> 100% local memory configuration
    # "50"  -> 50% local memory
    # "0"   -> 0% local memory  
    local exp_type=$1
    local run_cmd="${@:2}"
    echo "host node: $host_node, cxl_node: $cxl_node"
    # NOTE: fix mem_eater path like this, maybe need to change in the future
    local mem_eater="../build/memeater"
    if [[ $exp_type == "100" ]]; then 
        CPREFIX="numactl --cpunodebind $host_node --membind $host_node"
        run_cmd="numactl --cpunodebind $host_node --membind $host_node ""${run_cmd}"
    elif [[ $exp_type == "100" ]]; then 
        CPREFIX="numactl --cpunodebind $host_node --membind $cxl_node"
        run_cmd="numactl --cpunodebind $host_node --membind $cxl_node ""${run_cmd}"
    elif [[ $et == "CXL-Interleave" ]]; then
        CPREFIX="numactl --cpunodebind $host_node --interleave=all"
        run_cmd="numactl --cpunodebind $host_node --interleave=all ""${run_cmd}"
    else
        # Other base splits (e.g. 90, 80, 70, 60)
        run_cmd="numactl --cpunodebind 0 ${run_cmd}"
        NODE0_FREE_MEM=${node_free[$host_node]}
        CXL_FREE_MEM=${node_free[$cxl_node]}
        # reserve some memory for other utils programs
        NODE0_FREE_MEM=$((NODE0_FREE_MEM - 480))
        APP_MEM_ON_CXL=$(($CXL_FREE_MEM * $exp_type / 100))
        APP_MEM_ON_CXL=${APP_MEM_ON_CXL%.*}
        MEM_SHOULD_RESERVE=$((NODE0_FREE_MEM - APP_MEM_ON_CXL))
        MEM_SHOULD_RESERVE=${MEM_SHOULD_RESERVE%.*}
    fi

    echo "MEM_SHOULD_RESERVE: $MEM_SHOULD_RESERVE"
    # reserve some memory is necessary
    if [[ ${MEM_SHOULD_RESERVE} -gt 0 ]]; then
        echo "Reserve $MEM_SHOULD_RESERVE MB memory on node $host_node"
        killall memeater >/dev/null 2>&1
        sleep 10
        # make sure that MemEater is reserving memory from host node
        sudo numactl --cpunodebind $host_node --membind $host_node $mem_eater ${MEM_SHOULD_RESERVE} &
        mapid=$!
        # Wait until memory eater consume all destined memory
        sleep 60
    fi

    # run the target program
    echo "Run command: $run_cmd"
    $run_cmd
    # kill the memory eater
    if [[ $MEM_SHOULD_RESERVE -gt 0 ]]; then
        disown $mapid
        kill -9 $mapid >/dev/null 2>&1
    fi
    echo "Done command: $run_cmd"
    sleep 10
}   