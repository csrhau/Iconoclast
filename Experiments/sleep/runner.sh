#!/bin/bash -x

modprobe msr
BASEDIR=../..
DELAY=120
mkdir -p results

for FREQ in `$BASEDIR/Scripts/freq_avail.sh`; do
  echo Experiment: $FREQ
  for THREADCOUNT in $(seq 4 -1 1); do
    cpufreq-set -g performance --min $FREQ --max $FREQ -r
    cpufreq-info | grep 'current policy'
    ~sir/Dropbox/Work/Projects/Iconoclast/Tools/Power/build/power sleep $DELAY -t $THREADCOUNT > $(pwd)/results/freq_${FREQ}_threads_${THREADCOUNT}_sleep.out &
    sleep 3
  done
done
