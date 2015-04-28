#!/bin/bash -x

modprobe msr
BASEDIR=../..
mkdir -p results
BENCH=/home/sir/Projects/benchmarks/intel_linpack/l_lpk_p_11.2.2.010/linpack_11.2.2/benchmarks/linpack
ARCH=xeon64

for config in 1.60GHz:4 2.10GHz:3 2.30GHz:1 2.60GHz:2 3.00GHz:4 3.20GHz:1; do
    FREQ=${config%:*};
    CORES=${config#*:};
    cpufreq-set -g performance --min $FREQ --max $FREQ -r
    cpufreq-info | grep 'current policy'
    export OMP_NUM_THREADS=$CORES
    ~sir/Dropbox/Work/Projects/Iconoclast/Tools/Power/build/power $BENCH/xlinpack_xeon64 lininput_xeon64 > ./results/freq_${FREQ}_threads_${CORES}_power.out
done
