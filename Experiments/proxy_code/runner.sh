#!/bin/bash -x

modprobe msr
BASEDIR=../..
DELAY=120
mkdir -p results

for FREQ in `$BASEDIR/Scripts/freq_avail.sh | head -n 1`; do
  echo Experiment: $FREQ
  for THREADCOUNT in 4;do
    cpufreq-set -g performance --min $FREQ --max $FREQ -r
    cpufreq-info | grep 'current policy'
    ~sir/Dropbox/Work/Projects/Iconoclast/Tools/Power/build/power $BASEDIR/Benchmarks/proxy_code/build/lbench -u 100 -m 1000000 -t $THREADCOUNT > $(pwd)/results/freq_${FREQ}_threads_${THREADCOUNT}_power.out &
  sleep 3
  done
done
