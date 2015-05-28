#!/bin/bash -x

modprobe msr
BASEDIR=../..
RUN_LENGTH=100

mkdir -p results
BENCH=${BASEDIR}/Benchmarks/FIRESTARTER

for CORE_ID in 3 2 1 0; do
  THREADS=$(echo 1 + $CORE_ID | bc)
  for FREQ in `$BASEDIR/Scripts/freq_avail.sh`; do
    cpufreq-set -g performance --min $FREQ --max $FREQ -r
    cpufreq-info | grep 'current policy'
    sleep 10;
    time;
     ~sir/Dropbox/Work/Projects/Iconoclast/Tools/Power/build/power $BENCH/FIRESTARTER -t $RUN_LENGTH -b 0-$CORE_ID > results/freq-$FREQ-threads-$THREADS.out
  done
done
