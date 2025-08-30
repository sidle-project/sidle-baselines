#!/bin/bash

# check and set the total environment for running cxl-device
# refer from https://github.com/vtess/Pond/blob/master/cxl-global.sh

# ------------ begin some global functions ------------- #
get_sysinfo()
{
    uname -a
    echo "--------------------------"
    sudo numactl --hardware
    echo "--------------------------"
    lscpu
    echo "--------------------------"
    cat /proc/meminfo
}

flush_fs_caches()
{
    echo 3 | sudo tee /proc/sys/vm/drop_caches >/dev/null 2>&1
    sleep 10
}

disable_nmi_watchdog()
{
    echo 0 | sudo tee /proc/sys/kernel/nmi_watchdog >/dev/null 2>&1
}

disable_swap()
{
    sudo swapoff -a
}

disable_ksm()
{
    echo 0 | sudo tee /sys/kernel/mm/ksm/run >/dev/null 2>&1
}


disable_numa_balancing()
{
    echo 0 | sudo tee /proc/sys/kernel/numa_balancing >/dev/null 2>&1
}

# disable transparent hugepages
disable_thp()
{
    echo "never" | sudo tee /sys/kernel/mm/transparent_hugepage/enabled >/dev/null 2>&1
}

enable_turbo()
{
    echo 0 | sudo tee /sys/devices/system/cpu/intel_pstate/no_turbo >/dev/null 2>&1
}

disable_ht()
{
    echo off | sudo tee /sys/devices/system/cpu/smt/control >/dev/null 2>&1
}

disable_node1_cpus()
{
    echo 0 | sudo tee /sys/devices/system/node/node1/cpu*/online >/dev/null 2>&1
}

bring_all_cpus_online()
{
    echo 1 | sudo tee /sys/devices/system/cpu/cpu*/online >/dev/null 2>&1
}

disable_nodex_mem() 
{
    local node=$1
    echo 0 | sudo tee /sys/devices/system/node/node${node}/memory*/online >/dev/null 2>&1
}

disable_node1_mem()
{
    echo 0 | sudo tee /sys/devices/system/node/node1/memory*/online >/dev/null 2>&1
}

# set_performance_mode()
# {
#     #echo "  ===> Placing CPUs in performance mode ..."
#     for governor in /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor; do
#         echo performance | sudo tee $governor >/dev/null 2>&1
#     done
# }

# check_pmqos()
# {
#     local pmqospid=$(ps -ef | grep pmqos | grep -v grep | grep -v sudo | awk '{print $2}')
#     #echo $pmqospid

#     set_performance_mode
#     [[ -n "$pmqospid" ]] && return

#     sudo nohup ${TOPDIR}/pmqos >/dev/null 2>&1 &
#     sleep 3
#     # double check
#     pmqospid=$(ps -ef | grep pmqos | grep -v grep | grep -v sudo | awk '{print $2}')
#     if [[ -z "$pmqospid" ]]; then
#         echo "==> Error: failed to start pmqos!!!!"
#         exit
#     fi
# }

configure_base_exp_cores()
{
    # To be safe, let's bring all the cores online first
    echo 1 | sudo tee /sys/devices/system/cpu/cpu*/online >/dev/null 2>&1
    # Leave half of the cores online for both Node 0 and Node 1
	local cores_per_socket=$(lscpu | grep -i 'Core(s) per socket' | awk '{print $4}')
    local half_cores_per_socket=$((cores_per_socket / 2))
    local total_cores=$((cores_per_socket * 2))
    for ((i = half_cores_per_socket; i < cores_per_socket; i++)); do
        echo 0 | sudo tee /sys/devices/system/cpu/cpu$i/online >/dev/null 2>&1
    done

    for ((i = cores_per_socket + half_cores_per_socket; i < total_cores; i++)); do
        echo 0 | sudo tee /sys/devices/system/cpu/cpu$i/online >/dev/null 2>&1
    done
}

check_base_conf()
{
    disable_nmi_watchdog
    disable_ksm
    disable_numa_balancing
    disable_thp
    disable_ht
    configure_base_exp_cores
    # check_pmqos
    disable_swap

    nc=$(sudo numactl --hardware | grep 'node 1 cpus' | awk -F: '{print $2}')

    sleep 10
}

monitor_resource_util()
{
    while true; do
        local o=$(sudo numactl --hardware)
        local node0_free_mb=$(echo "$o" | grep "node 0 free" | awk '{print $4}')
		local node1_free_mb=$(echo "$o" | grep "node 1 free" | awk '{print $4}')
        echo "$(date +"%D %H%M%S") ${node0_free_mb} ${node1_free_mb}"
        #pidstat -r -u -d -l -p ALL -U -h 5 100000000 > pidstat.log &
        sleep 5
    done
}


# ------------ end some global functions ------------- #

