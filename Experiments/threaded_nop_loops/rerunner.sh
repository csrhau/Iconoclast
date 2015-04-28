#!/bin/bash -x

modprobe msr
BASEDIR=../..
DELAY=120
mkdir -p results

for run in `seq 2 4`; do 
for FREQ in 2.50GHz; do
  echo Experiment: $FREQ
  for THREADCOUNT in 1; do
    cpufreq-set -g performance --min $FREQ --max $FREQ -r
    cpufreq-info | grep 'current policy'
    ~sir/Dropbox/Work/Projects/Iconoclast/Tools/Power/build/power $BASEDIR/Benchmarks/threaded_empty_loops/build/lbench -t $THREADCOUNT > $(pwd)/results/freq_${FREQ}_threads_${THREADCOUNT}_power.out.${run} &
    sleep $DELAY
    pkill lbench
    sleep 3
  done
done
done
