#!/bin/bash

# enable, disable, make_then_disable, make_then_enable
prep="disable"
count=1
benchmark="mix"
# baseline, baseline_random_cancel, failover, failover_random_cancel
policy="baseline_random_cancel" 
outfile="${policy}_${benchmark}.out"

if [ "$prep" = "disable" ]; then
    sudo ./disable_linnos.sh
elif [ "$prep" = "make_then_disable" ]; then
    cd ..
    sudo make
    cd kernel_hook
    echo "building kernel_hook main"
    sudo make clean all
    sudo ./disable_linnos.sh
elif [ "$prep" = "enable" ]; then
    sudo ./disable_linnos.sh
    sudo ./enable_linnos_gpu.sh
elif [ "$prep" = "make_then_enable" ]; then
    cd ..
    sudo make
    cd kernel_hook
    echo "building kernel_hook main"
    sudo make clean all
    sudo ./disable_linnos.sh
    sudo ./enable_linnos_gpu.sh
fi

# remake io_replayer
cd ../io_replayer
sudo make

if [ "$benchmark" != "mix" ]; then
    echo "first"
    # for all benchmarks that are not mix
    for i in $(seq $count); do 
        echo "Run $i"
        ./run_3ssds.sh ${policy} ../trace_tools/${benchmark}/${benchmark}1.trace \ ../trace_tools/${benchmark}/${benchmark}1.trace \ ../trace_tools/${benchmark}/${benchmark}.trace
        cd ../io_replayer
        python3 stats.py 3ssds_${policy}.data>>${outfile}
        sudo rm 3ssds_${policy}.data
    done
else 
    echo "second"
    for i in $(seq $count); do 
        echo "Run $i"
        ./run_3ssds.sh ${policy} ../trace_tools/azure/azure1.trace \ ../trace_tools/bing_i/bing_i1.trace \ ../trace_tools/cosmos/cosmos1.trace
        cd ../io_replayer
        python3 stats.py 3ssds_${policy}.data>>${outfile}
        sudo rm 3ssds_${policy}.data
    done
fi