#!/bin/bash -x

modprobe msr
BASEDIR=../..
mkdir -p results
BENCH=/home/sir/Projects/benchmarks/intel_linpack/l_lpk_p_11.2.2.010/linpack_11.2.2/benchmarks/linpack
ARCH=xeon64

for THREADCOUNT in 4 3 2 1; do
  for FREQ in `$BASEDIR/Scripts/freq_avail.sh`; do
    cpufreq-set -g performance --min $FREQ --max $FREQ -r
    cpufreq-info | grep 'current policy'
    export OMP_NUM_THREADS=$THREADCOUNT
    ~sir/Dropbox/Work/Projects/Iconoclast/Tools/Power/build/power $BENCH/xlinpack_xeon64 lininput_xeon64 > ./results/freq_${FREQ}_threads_${THREADCOUNT}_power.out
  done
done
