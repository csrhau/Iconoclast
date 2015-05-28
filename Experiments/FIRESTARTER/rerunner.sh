#!/bin/bash -x

modprobe msr
BASEDIR=../..
RUN_LENGTH=100

mkdir -p results
BENCH=${BASEDIR}/Benchmarks/FIRESTARTER

for config in 3.20GHz:3 2.90GHz:2 3.20GHz:0;  do
  FREQ=${config%:*}
  CORE_ID=${config#*:}
  THREADS=$(echo 1 + $CORE_ID | bc)
    cpufreq-set -g performance --min $FREQ --max $FREQ -r
    cpufreq-info | grep 'current policy'
    sleep 10;
    time;
     ~sir/Dropbox/Work/Projects/Iconoclast/Tools/Power/build/power $BENCH/FIRESTARTER -t $RUN_LENGTH -b 0-$CORE_ID > results/freq-$FREQ-threads-$THREADS.out
done
