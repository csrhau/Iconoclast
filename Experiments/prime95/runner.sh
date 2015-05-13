#!/bin/bash -x

modprobe msr
BASEDIR=../..
DELAY=180
mkdir -p results
BENCH=/home/sir/Projects/benchmarks/prime95

for FREQ in `$BASEDIR/Scripts/freq_avail.sh`; do
  echo Experiment: $FREQ
  for THREADCOUNT in 1 2 3 4; do
    cpufreq-set -g performance --min $FREQ --max $FREQ -r
    cpufreq-info | grep 'current policy'
    ~sir/Dropbox/Work/Projects/Iconoclast/Tools/Power/build/power ./shim.exp $THREADCOUNT $DELAY>  $(pwd)/results/freq_${FREQ}_threads_${THREADCOUNT}_power.out 
    sleep 6
  done
done
